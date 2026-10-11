#include <atomic>
#include <chrono>
#include <thread>
#include <iostream>
#include <cstdint>
#include <vector>
#include <thread>
#include <cassert>
#include <sstream>
#include <mutex>
#include <iomanip>

// Enforce 4-byte object alignment, which guarantees
// that the pointer's two least significant bits are always 0.
template <typename T>
struct alignas(4) ExchangedNode {
    T payload;
    explicit ExchangedNode(T val) : payload(val) {}
};

template <typename T>
class LockFreeExchanger {
private:
    // State machine states
    static constexpr uintptr_t EMPTY = 0;
    static constexpr uintptr_t WAITING = 1;
    static constexpr uintptr_t BUSY = 2;
    static constexpr uintptr_t STATE_MASK = 3;
    // Atomic variable holding both the pointer and the state
    std::atomic<uintptr_t> slot{EMPTY};
    // HELPER FUNCTIONS (Tagged Pointers)
    static uintptr_t pack(ExchangedNode<T>* ptr, uintptr_t state) {
        return reinterpret_cast<uintptr_t>(ptr) | state;
    }
    static ExchangedNode<T>* extract_ptr(uintptr_t val) {
        return reinterpret_cast<ExchangedNode<T>*>(val & ~STATE_MASK);
    }
    static uintptr_t extract_state(uintptr_t val) {
        return val & STATE_MASK;
    }
public:
    // Attempt an exchange.
    // Return the partner's node pointer (success) or nullptr (timeout).
    ExchangedNode<T>* exchange(ExchangedNode<T>* my_node, std::chrono::microseconds timeout) {
        auto start_time = std::chrono::steady_clock::now();
        while (true) {
            if (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start_time) >= timeout) {
                return nullptr;
            }
            uintptr_t current = slot.load();
            uintptr_t state = extract_state(current);
            ExchangedNode<T>* partner_node = extract_ptr(current);
            if (state == EMPTY) {
                // "sit down at the table"
                uintptr_t waiting_msg = pack(my_node, WAITING);
                if (slot.compare_exchange_strong(current, waiting_msg)) {
                    // The state was indeed EMPTY
                    // We set WAITING; now wait for someone to change the state to BUSY
                    while (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start_time) < timeout) {
                        uintptr_t slot_now = slot.load();
                        uintptr_t state_now = extract_state(slot_now);
                        if (state_now == BUSY) {
                            // Someone changed the state to BUSY, so return their pointer
                            ExchangedNode<T>*  partner = extract_ptr(slot_now);
                            slot.store(EMPTY);
                            return partner;
                        }
                        // Yield CPU time so another thread can change the state
                        std::this_thread::yield();
                    }
                    // Timeout - clean up
                    if (slot.compare_exchange_strong(waiting_msg, EMPTY)) {
                        // No partner arrived
                        return nullptr;
                    } else { // A partner arrived at the last moment; return the pointer in the BUSY state and clear the slot
                        uintptr_t final_msg = slot.load();
                        ExchangedNode<T>*  partner = extract_ptr(final_msg);
                        slot.store(EMPTY);
                        return partner;
                    }
                }
            } else if (state == WAITING) {
                // A partner was already waiting
                uintptr_t busy_msg = pack(my_node, BUSY);
                if (slot.compare_exchange_strong(current, busy_msg)) {
                    // current contained the partner's address, extracted earlier by extract_ptr(current)
                    return partner_node;
                }
            } else if (state == BUSY) {
                std::this_thread::yield();
            }
        }
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

void test_exchanger() {
    LockFreeExchanger<int> exchanger;

    // Basic exchange
    {
        SafePrint() << "TEST 1: Starting an exchange between two threads\n";
        ExchangedNode<int> node1(10);
        ExchangedNode<int> node2(20);

        std::jthread t1([&]() {
            auto res = exchanger.exchange(&node1, std::chrono::milliseconds(500));
            if (res) SafePrint() << "Thread 1 received: " << res->payload << "\n";
            else SafePrint() << "Thread 1: TIMEOUT\n";
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Give t1 time to wait on the "bench"

        std::jthread t2([&]() {
            auto res = exchanger.exchange(&node2, std::chrono::milliseconds(500));
            if (res) SafePrint() << "Thread 2 received: " << res->payload << "\n";
            else SafePrint() << "Thread 2: TIMEOUT\n";
        });
    }

    // TEST 2: Timeout (no partner arrives)
    {
        SafePrint() << "\nTEST 2: Timeout test\n";
        ExchangedNode<int> node1(999);
        auto start = std::chrono::steady_clock::now();

        auto res = exchanger.exchange(&node1, std::chrono::milliseconds(200));
        auto end = std::chrono::steady_clock::now();

        if (res == nullptr) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            SafePrint() << "Thread correctly returned after " << duration << " ms with nullptr.\n";
        }
    }

    // TEST 3: Repeated use
    {
        SafePrint() << "\nTEST 3: Object reuse test\n";
        ExchangedNode<int> n1(1), n2(2), n3(3), n4(4);

        // First pair
        std::jthread tA([&](){ exchanger.exchange(&n1, std::chrono::milliseconds(100)); });
        std::jthread tB([&](){ exchanger.exchange(&n2, std::chrono::milliseconds(100)); });
        tA.join(); tB.join();

        // Second pair (the same exchanger)
        std::jthread tC([&](){
            auto res = exchanger.exchange(&n3, std::chrono::milliseconds(100));
            if(res) SafePrint() << "Second exchange OK, received: " << res->payload << "\n";
        });
        std::jthread tD([&](){ exchanger.exchange(&n4, std::chrono::milliseconds(100)); });
    }
}

int main() {
    test_exchanger();
    return 0;
}
