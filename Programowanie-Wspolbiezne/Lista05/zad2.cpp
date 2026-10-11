#include <array>
#include <atomic>
#include <optional>
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <iomanip>

template<typename T, size_t Size>  // Size must be a power of two; the queue stores one fewer element
class Queue {
private:
    std::array<T, Size> buff;
    std::atomic<size_t> head{0}, tail{0};
public:

    Queue() = default;

    static_assert((Size != 0) && ((Size & (Size - 1)) == 0), "Size needs to be a power of two.");

    bool push(const T& val) {
        size_t current_head = head.load(std::memory_order_acquire);
        size_t current_tail = tail.load(std::memory_order_relaxed);
        size_t new_tail = (current_tail + 1) & (Size - 1);

        if (new_tail == current_head){
            return false;
        }

        buff[current_tail] = val;
        tail.store(new_tail, std::memory_order_release);
        return true;
    }

    std::optional<T> pop() {
        size_t current_head = head.load(std::memory_order_relaxed);
        size_t current_tail = tail.load(std::memory_order_acquire);

        if (current_head == current_tail) {
            return std::nullopt;
        }

        std::optional<T> opt{buff[current_head]};
        size_t new_head = (current_head + 1) & (Size - 1);
        head.store(new_head, std::memory_order_release);
        return opt;
    }
};

// Global program start time (for measuring elapsed time)
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

void test_edge_cases() {
    SafePrint() << "\n--- EDGE CASE TESTS ---\n";
    {
        Queue<int, 1> q; // This queue cannot hold any data

        bool push_full = q.push(1);

        SafePrint() << "1. Test `push` on a full queue: " << (!push_full ? "[PASSED]" : "[FAILED]") << "\n";

        auto pop_empty = q.pop();

        SafePrint() << "2. Test `pop` on an empty queue: " << (!pop_empty.has_value() ? "[PASSED]" : "[FAILED]") << "\n";
    }
    {

        Queue<int, 4> q;

        bool push_1 = q.push(1);
        bool push_2 = q.push(2);
        bool push_3 = q.push(3);
        bool push_full = q.push(4);

        bool push_ok = (push_1 && push_2 && push_3) && !push_full;

        auto pop_3 = (q.pop().value() == 1);
        auto pop_2 = (q.pop().value() == 2);
        auto pop_1 = (q.pop().value() == 3);
        auto pop_empty = !(q.pop().has_value());

        bool pop_ok = pop_3 && pop_2 && pop_1 && pop_empty;

        bool passed = (push_ok && pop_ok);

        SafePrint() << "2. Test 4x`push` and 4x`pop` on a queue with capacity 3: " << (passed ? "[PASSED]" : "[FAILED]") << "\n";
    }
    {
        Queue<int, 4> q;

        q.push(10);
        q.push(20);
        q.push(30);

        q.pop(); // Remove 10
        q.pop(); // Remove 20

        // tail is now at the end of the array. Subsequent push operations
        // must wrap around to indices 0 and 1.
        bool push_40 = q.push(40);
        bool push_50 = q.push(50);
        bool push_full = q.push(60); // This should be rejected again!

        // Check that reading also wraps head back to the beginning correctly
        auto val_30 = q.pop().value_or(0);
        auto val_40 = q.pop().value_or(0);
        auto val_50 = q.pop().value_or(0);

        bool wrap_ok = push_40 && push_50 && !push_full;
        bool values_ok = (val_30 == 30) && (val_40 == 40) && (val_50 == 50);

        SafePrint() << "3. Index wrap-around test: "
                    << ((wrap_ok && values_ok) ? "[PASSED]" : "[FAILED]") << "\n";
    }
    {
        Queue<int, 4> q;
        bool marathon_ok = true;

        // Perform 100 operations on a queue with capacity 3.
        // The indices will wrap around the array 25 times
        for (int i = 0; i < 100; ++i) {
            bool pushed = q.push(i);
            auto popped = q.pop();

            if (!pushed || popped.value_or(-1) != i) {
                marathon_ok = false;
                break;
            }
        }

        SafePrint() << "4. Interleaving test (100x push/pop): "
                    << (marathon_ok ? "[PASSED]" : "[FAILED]") << "\n";
    }
}

void test_multithreaded_stress() {
    SafePrint() << "\n--- MULTITHREADED TEST ---\n";
    Queue<int, 1024> q;
    const int ELEMENTS = 1'000'000;
    std::atomic<int> errors{0};

    // Producer: push numbers from 1 to 1,000,000
    auto producer = [&]() {
        for (int i = 1; i <= ELEMENTS; ++i) {
            while (!q.push(i)) {
                // Buffer full - busy-wait
            }
        }
    };

    // Consumer: retrieve numbers and verify their order
    auto consumer = [&]() {
        int expected = 1;
        while (expected <= ELEMENTS) {
            if (auto val = q.pop()) {
                if (val.value() != expected) {
                    errors++; // Synchronization error detected!
                }
                expected++;
            }
        }
    };

    std::jthread prod(producer);
    std::jthread cons(consumer);

    cons.join();
    prod.join();

    SafePrint() << "1. Test FIFO: "
                << (errors == 0 ? "[PASSED]" : "[FAILED]")
                << " (Errors: " << errors << ")\n";
}

void test_multithreaded_jitter() {
    SafePrint() << "\n--- ADVERSARIAL SCHEDULER TEST ---\n";
    Queue<int, 16> q;
    const int ELEMENTS = 100'000;
    std::atomic<int> errors{0};

    auto producer = [&]() {
        for (int i = 1; i <= ELEMENTS; ++i) {
            while (!q.push(i)) {
                // Simulate the operating system preempting this thread at the worst possible moment
                if (i % 5 == 0) {
                    std::this_thread::yield();
                }
            }
        }
    };

    auto consumer = [&]() {
        int expected = 1;
        while (expected <= ELEMENTS) {
            if (auto val = q.pop()) {
                if (val.value() != expected) {
                    errors++;
                }
                expected++;
            } else {
                // Simulate a sleeping consumer so the producer runs into a full buffer
                if (expected % 3 == 0) {
                    std::this_thread::yield();
                }
            }
        }
    };

    std::jthread prod(producer);
    std::jthread cons(consumer);

    cons.join();
    prod.join();

    SafePrint() << "Jitter test: "
                << (errors == 0 ? "[PASSED]" : "[FAILED]")
                << " (Errors: " << errors << ")\n";
}

int main() {

    test_edge_cases();

    test_multithreaded_stress();
    test_multithreaded_jitter();

    SafePrint() << "\nAll test procedures finished!\n";
    return 0;
}
