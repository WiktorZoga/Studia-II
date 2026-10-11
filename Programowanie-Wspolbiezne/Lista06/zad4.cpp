#include <atomic>
#include <optional>
#include <vector>
#include <thread>
#include <numeric>
#include <set>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <mutex>

template<typename T>
class StealingQueue {
private:
    std::vector<T> queue;
    std::atomic<int64_t> top{0}, bottom{0}; // track the ordering of top and bottom; use modulo for buffer indexing
    int64_t const MAX_SIZE;
public:
    StealingQueue(int64_t size): MAX_SIZE(size), queue(size) {}

    StealingQueue(const StealingQueue&) = delete;
    StealingQueue& operator=(const StealingQueue&) = delete;

    StealingQueue(StealingQueue&&) = delete;
    StealingQueue& operator=(StealingQueue&&) = delete;

    void push(T item) {
        // only the owner modifies bottom, so its local load can be relaxed
        int64_t b = bottom.load(std::memory_order_relaxed);

        // acquire synchronizes with the thieves' release operations.
        // ensures visibility of top updates from successful steals.
        int64_t t = top.load(std::memory_order_acquire);

        if (b - t >= MAX_SIZE) return; // guard against buffer overflow

        queue[b % MAX_SIZE] = std::move(item);

        // ensures that the data written to the array
        // is fully visible BEFORE bottom is incremented.
        bottom.store(b + 1, std::memory_order_release);
    }

    std::optional<T> pop() {
        int64_t b = bottom.load(std::memory_order_relaxed) - 1;

        // ensure the thieves see that we have reserved the element
        // BEFORE we check their position.
        bottom.store(b, std::memory_order_seq_cst);

        // the processor must not reorder the bottom store and top load
        int64_t t = top.load(std::memory_order_seq_cst);

        if (t < b) {
            // Safe region: the thieves are far away, so the element is ours.
            return std::move(queue[b % MAX_SIZE]);
        }

        std::optional<T> result = std::nullopt;

        if (t == b) {
            // last element
            if (top.compare_exchange_strong(t, t + 1,
                                            std::memory_order_seq_cst,
                                            std::memory_order_relaxed)) {
                // We win the race
                result = std::move(queue[b % MAX_SIZE]);
            }

            // the new top is t + 1.
            // the deque is now empty; bottom must catch up
            // to prevent the indices from becoming inverted (bottom < top).
            bottom.store(t + 1, std::memory_order_relaxed);
            return result;
        }

        // t > b: the deque was empty
        bottom.store(t, std::memory_order_relaxed);
        return std::nullopt;
    }

    std::optional<T> steal() {
        // load top, then bottom.
        // seq_cst prevents these operations from being reordered
        int64_t t = top.load(std::memory_order_seq_cst);
        int64_t b = bottom.load(std::memory_order_seq_cst);

        if (t < b) {
            // we cannot use std::move here
            // if a thread moved the value and then lost the CAS
            // to another thief, the winner could receive a moved-from object
            // because we would already have moved its data into our item variable
            T item = queue[t % MAX_SIZE];

            // win the race and take the copied element
            if (top.compare_exchange_strong(t, t + 1,
                                            std::memory_order_seq_cst,
                                            std::memory_order_relaxed)) {
                return item;
            }
        }
        return std::nullopt;
    }
};

inline const auto start_time = std::chrono::high_resolution_clock::now();

class SafePrint {
public:
    SafePrint() {
        auto now = std::chrono::high_resolution_clock::now();
        auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time).count();
        buffer << "[" << std::setw(8) << micros << " us] ";
    }

    ~SafePrint() {
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << buffer.str() << std::flush;
    }

    template<typename T>
    SafePrint& operator<<(const T& msg) {
        buffer << msg;
        return *this;
    }

private:
    static std::mutex mtx;
    std::ostringstream buffer;
};

std::mutex SafePrint::mtx;

void test_logic() {
    StealingQueue<int> q(16);
    q.push(1);
    q.push(2);
    q.push(3);

    auto val1 = q.pop();   // LIFO: should be 3
    auto val2 = q.steal(); // FIFO: should be 1

    if (val1 == 3 && val2 == 1) {
        SafePrint() << "Basic test: OK\n";
    } else {
        SafePrint() << "Basic test: FAILED\n";
    }
}

void stress_test() {
    const int num_items = 100000;
    const int num_thieves = 4;
    StealingQueue<int> q(num_items * 2);

    std::atomic<int> items_processed{0};
    std::atomic<long long> sum_processed{0};
    std::atomic<bool> done{false};

    std::vector<std::jthread> thieves;
    for (int i = 0; i < num_thieves; ++i) {
        thieves.emplace_back([&]() {
            while (!done || items_processed < num_items) {
                if (auto item = q.steal()) {
                    items_processed++;
                    sum_processed += *item;
                }
                std::this_thread::yield();
            }
        });
    }

    long long expected_sum = 0;
    for (int i = 1; i <= num_items; ++i) {
        q.push(i);
        expected_sum += i;

        if (i % 10 == 0) {
            if (auto item = q.pop()) {
                items_processed++;
                sum_processed += *item;
            }
        }
    }

    done = true;
    thieves.clear();

    SafePrint() << "Stress test results\n";
    SafePrint() << "Items submitted:   " << num_items << "\n";
    SafePrint() << "Items processed:   " << items_processed << "\n";
    SafePrint() << "Expected sum:      " << expected_sum << "\n";
    SafePrint() << "Actual sum:        " << sum_processed << "\n";

    if (sum_processed == expected_sum) {
        SafePrint() << "STATUS: TEST PASSED (no mismatches detected)\n";
    } else {
        SafePrint() << "STATUS: FAILED! The sums do not match!\n";
    }
}

void test_conflict() {
    StealingQueue<int> q(1024);
    std::atomic<int> success_count{0};

    for (int i = 0; i < 50000; ++i) {
        q.push(99);

        std::jthread t([&]() {
            if (q.steal()) success_count++;
        });

        if (q.pop()) success_count++;

        t.join();

        if (success_count != i + 1) {
            SafePrint() << "Failure in iteration " << i << "! An element was lost or duplicated.\n";
            return;
        }
    }
    SafePrint() << "Contention test: OK (50k iterations without failures)\n";
}

int main() {
    test_logic();
    test_conflict();
    stress_test();
    return 0;
}
