#include <atomic>
#include <chrono>
#include <thread>
#include <cstdint>
#include <vector>
#include <cassert>
#include <mutex>
#include <memory>
#include <optional>
#include <random>
#include <stop_token>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>

enum class Type { PUSH, POP };

// Węzeł Exchangera - dodany Type, żeby wykryć push-push / pop-pop i nie dopuścić do wymiany
template <typename T>
struct alignas(4) ExchangedNode {
    std::optional<T> payload;
    Type type;
    explicit ExchangedNode(std::optional<T> val, Type t) : payload(std::move(val)), type(t) {}
};

template <typename T>
class LockFreeExchanger {
private:
    static constexpr uintptr_t EMPTY = 0;
    static constexpr uintptr_t WAITING = 1;
    static constexpr uintptr_t BUSY = 2;
    static constexpr uintptr_t STATE_MASK = 3;
    
    std::atomic<uintptr_t> slot{EMPTY};

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
                uintptr_t waiting_msg = pack(my_node, WAITING);
                if (slot.compare_exchange_strong(current, waiting_msg)) {
                    while (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start_time) < timeout) {
                        uintptr_t slot_now = slot.load();
                        if (extract_state(slot_now) == BUSY) {
                            ExchangedNode<T>* partner = extract_ptr(slot_now);
                            slot.store(EMPTY);
                            return partner;
                        }
                        std::this_thread::yield();
                    }
                    if (slot.compare_exchange_strong(waiting_msg, EMPTY)) {
                        return nullptr;
                    } else { 
                        uintptr_t final_msg = slot.load();
                        ExchangedNode<T>* partner = extract_ptr(final_msg);
                        slot.store(EMPTY);
                        return partner;
                    }
                }
            } else if (state == WAITING) {
                // Zapobiega wymianie push-push / pop-po i utracie danych
                if (my_node->type == partner_node->type) {
                    return nullptr; 
                }

                uintptr_t busy_msg = pack(my_node, BUSY);
                if (slot.compare_exchange_strong(current, busy_msg)) {
                    return partner_node;
                }
            } else if (state == BUSY) {
                std::this_thread::yield(); 
            }
        }
    }
};

template<typename T>
class EliminationArray {
private:
    std::vector<LockFreeExchanger<T>> exchanger;
    std::chrono::microseconds duration;
    int capacity;
    inline static thread_local std::mt19937 generator{std::random_device{}()}; 

public:
    EliminationArray(int _capacity, std::chrono::microseconds _duration) : 
        capacity(_capacity), duration(_duration), exchanger(_capacity) {}
    
    ExchangedNode<T>* visit(ExchangedNode<T>* my_node) {
        std::uniform_int_distribution<int> distribution(0, capacity - 1);
        int slot = distribution(generator); 
        return exchanger[slot].exchange(my_node, duration);
    }
};

template<typename T>
class LockFreeStack {
private:
    struct Node {
        T data;
        std::shared_ptr<Node> next;
        Node(T val) : data(std::move(val)), next(nullptr) {}
    };

    EliminationArray<T> elimination;
    std::shared_ptr<Node> head{nullptr};

public: 
    LockFreeStack(int _capacity, std::chrono::microseconds _duration) : elimination(_capacity, _duration) {}

    void push(T val) {
        auto new_node = std::make_shared<Node>(val);
        ExchangedNode<T>* my_box = nullptr; 
        
        while (true) {
            auto current_head = std::atomic_load_explicit(&head, std::memory_order_relaxed);
            new_node->next = current_head;
            
            if (std::atomic_compare_exchange_weak_explicit(&head, &current_head, new_node, std::memory_order_release, std::memory_order_relaxed)) {
                if (my_box) delete my_box; 
                return;
            } else {
                if (!my_box) my_box = new ExchangedNode<T>(val, Type::PUSH);
                
                ExchangedNode<T>* partner_box = elimination.visit(my_box);
                
                if (partner_box != nullptr) {
                    delete partner_box;
                    return;
                }
            }
        }
    }

    std::optional<T> pop() {
        ExchangedNode<T>* my_box = nullptr;

        while (true) {
            auto current_head = std::atomic_load_explicit(&head, std::memory_order_acquire);
            if (current_head == nullptr) { 
                if (my_box) delete my_box; 
                return std::nullopt;
            }
            
            if (std::atomic_compare_exchange_weak_explicit(&head, &current_head, current_head->next, std::memory_order_release, std::memory_order_acquire)) {
                if (my_box) delete my_box; 
                return current_head->data; 
            } else {
                if (!my_box) my_box = new ExchangedNode<T>(std::nullopt, Type::POP);
                
                ExchangedNode<T>* partner_box = elimination.visit(my_box);
                
                if (partner_box != nullptr) {
                    std::optional<T> result = partner_box->payload;
                    delete partner_box;
                    return result;
                }
            }
        }
    }
};

void benchmark_producer_consumer(int num_threads, int capacity, int timeout_us) {
    LockFreeStack<int> stack(capacity, std::chrono::microseconds(timeout_us));

    int actual_threads = std::max(2, num_threads);
    int num_producers = actual_threads / 2;
    int num_consumers = actual_threads - num_producers;

    const int ITEMS_PER_PRODUCER = 100'000;
    const int TOTAL_ITEMS = num_producers * ITEMS_PER_PRODUCER;

    std::atomic<int> items_consumed{0};

    auto producer = [&stack, ITEMS_PER_PRODUCER]() {
        for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
            stack.push(i);
        }
    };

    auto consumer = [&stack, &items_consumed, TOTAL_ITEMS]() {
        while (items_consumed.load(std::memory_order_relaxed) < TOTAL_ITEMS) {
            auto val = stack.pop();
            if (val.has_value()) {
                items_consumed.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield(); 
            }
        }
    };

    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;
    for (int i = 0; i < num_consumers; i++) {
        threads.emplace_back(consumer);
    }
    for (int i = 0; i < num_producers; i++) {
        threads.emplace_back(producer);
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    double total_operations = TOTAL_ITEMS * 2.0; 
    double ops_per_sec = (duration > 0) ? (total_operations * 1000.0) / duration : 0.0;

    std::cout << "Prod/Cons (Watki: " << std::setw(2) << actual_threads 
              << ", Cap: " << std::setw(2) << capacity 
              << ", Time: " << std::setw(3) << timeout_us << "us) -> " 
              << std::setw(4) << duration << " ms | "
              << std::fixed << std::setprecision(0) << ops_per_sec << " ops/sec\n";
}

int main() {
    unsigned int max_threads = std::thread::hardware_concurrency();
    std::cout << "Maksymalna liczba watkow sprzetowych: " << max_threads << "\n\n";

    std::vector<int> thread_counts = {2, 4, 8, 10};

    int test_capacity = 4; 
    int test_timeout_us = 20; 

    std::cout << "Test Producent/Konsument (Lacznie ~" << (thread_counts.back() / 2) * 100'000 << " operacji)\n";
    
    for (int t : thread_counts) {
        benchmark_producer_consumer(t, test_capacity, test_timeout_us);
    }

    return 0;
}

/*

Moje wyniki

./zad3
Maksymalna liczba watkow sprzetowych: 10

Test Producent/Konsument (Lacznie ~500000 operacji)
Prod/Cons (Watki:  2, Cap:  4, Time:  20us) ->   97 ms
Prod/Cons (Watki:  4, Cap:  4, Time:  20us) ->  203 ms
Prod/Cons (Watki:  8, Cap:  4, Time:  20us) ->  718 ms
Prod/Cons (Watki: 10, Cap:  4, Time:  20us) -> 1004 ms


*/