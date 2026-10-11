#include <atomic>
#include <thread>
#include <iostream>
#include <vector>
#include <cassert>
#include <stop_token>

class TicketLock {
private:
    std::atomic<size_t> next_ticket{0};
    std::atomic<size_t> now_serving{0};

public:
    void lock() {
        size_t my_ticket = next_ticket.fetch_add(1, std::memory_order_relaxed); // we only need atomicity

        while (now_serving.load(std::memory_order_acquire) != my_ticket) {
            // synchronize with unlock and keep critical-section operations after lock acquisition
            std::this_thread::yield();
        }
    }

    void unlock() {
        now_serving.fetch_add(1, std::memory_order_release);
        // ensure all critical-section operations complete before releasing the lock
    }
};

// overflow wraps modulo 2^N; this is not a problem unless there are 2^N outstanding tickets

TicketLock ticket_lock;

// shared variable protected by the lock
size_t shared_counter = 0;

int const NUM_THREADS = 8;
int const INCREMENTS_PER_THREAD = 100000;

void worker_task(std::stop_token stoken) {

    for (int i = 0; i < INCREMENTS_PER_THREAD && !stoken.stop_requested(); ++i) {
        ticket_lock.lock();

        // critical section
        shared_counter++;

        ticket_lock.unlock();
    }
}

int main() {

    // The tests passed even when I used relaxed everywhere...

    std::cout << "Testing TicketLock...\n";

    std::vector<std::jthread> threads;

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker_task);
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

    for (auto& t : threads) {
        t.request_stop();
    }

    size_t expected = NUM_THREADS * INCREMENTS_PER_THREAD;
    std::cout << "Expected value:     " << expected << "\n";
    std::cout << "Actual value:       " << shared_counter << "\n";

    if (shared_counter == expected) {
        std::cout << "RESULT: SUCCESS!\n";
    } else {
        std::cout << "RESULT: DATA RACE! Difference: "
                  << (expected - shared_counter) << " operations\n";
    }

    return 0;
}
