#include <atomic>
#include <thread>
#include <utility>
#include <iostream>
#include <vector>
#include <chrono>

template<typename T>
class SeqLock { // especially efficient with many readers and very fast copying
private:
    std::atomic<T> payload;
    std::atomic<size_t> seq{0};

public:
    SeqLock() = default;

    // void write(const T& val) {
    //     seq.fetch_add(1, std::memory_order_acquire);  // the acquire barrier keeps the payload store after marking the state odd (modification in progress)
    //     payload.store(val, std::memory_order_relaxed);
    //     seq.fetch_add(1, std::memory_order_release); // ensure the data is stored before publishing the new counter
    // }

    void write(const T& val) {
        size_t s = seq.load(std::memory_order_relaxed);
        seq.store(s + 1, std::memory_order_relaxed);

        std::atomic_thread_fence(std::memory_order_release);

        payload.store(val, std::memory_order_relaxed);

        seq.store(s + 2, std::memory_order_release);
    }

    T read() const {
        while (true) {
            size_t current_seq = seq.load(std::memory_order_acquire);
            if (current_seq % 2 == 0) {
                T result = payload.load(std::memory_order_relaxed);

                std::atomic_thread_fence(std::memory_order_acquire);

                if (current_seq == seq.load(std::memory_order_relaxed)) {
                    return result;
                }
            }
            std::this_thread::yield();
        }
    }

};

// The test structure has several fields to help detect inconsistent writes
struct alignas(16) TestData {
    size_t a;
    size_t b;
    size_t c;
};

// Compile-time check that the structure meets the SeqLock type requirement
// The type must be trivially copyable so a collision only risks mixed bytes
// rather than memory allocation errors
static_assert(std::is_trivially_copyable_v<TestData>, "TestData must be trivially copyable!");

SeqLock<TestData> global_seqlock;
std::atomic<size_t> fails_detected{0};
std::atomic<size_t> successful_reads{0};

void writer_task(std::stop_token stoken) {
    size_t counter = 0;
    while (!stoken.stop_requested()) {
        counter++;
        global_seqlock.write({counter, counter, counter});
    }
}

void reader_task(std::stop_token stoken) {
    size_t local_success_count = 0;
    while (!stoken.stop_requested()) {
        TestData data = global_seqlock.read();
        if (data.a != data.b || data.b != data.c) { // fail!
            fails_detected.fetch_add(1, std::memory_order_relaxed);
        } else {
            local_success_count++;
        }
    }
    // update the shared count at the end for speed and safety
    successful_reads.fetch_add(local_success_count, std::memory_order_relaxed);
}

int main() { // using relaxed for the first load in read causes failures!
    std::cout << "Testing SeqLock...\n";

    global_seqlock.write({0, 0, 0});

    std::jthread writer(writer_task);

    std::vector<std::jthread> readers;
    for (int i = 0; i < 5; ++i) {
        readers.emplace_back(reader_task);
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

    writer.request_stop();
    for (auto& r : readers) {
        r.request_stop();
    }

    std::cout << "Test completed.\n";
    std::cout << "Successful reads: " << successful_reads.load() << "\n";
    std::cout << "Failures: " << fails_detected.load() << "\n";

    return 0;
}
