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

template <typename T>
struct alignas(4) ExchangedNode {
    // T payload;
    std::optional<T> payload;
    explicit ExchangedNode(std::optional<T> val) : payload(val) {}
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
    // sukces, wartość od partnera (w taki sposob do wartość od partnera też może być std::nullopt)
    std::pair<bool, std::optional<T>> exchange(ExchangedNode<T>* my_node, std::chrono::microseconds timeout) {
    // ExchangedNode<T>* exchange(ExchangedNode<T>* my_node, std::chrono::microseconds timeout) {
        auto start_time = std::chrono::steady_clock::now();
        while (true) {
            if (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start_time) >= timeout) {
                return {false, std::nullopt};
                // return nullptr;
            }
            uintptr_t current = slot.load(std::memory_order_relaxed);  
            uintptr_t state = extract_state(current);
            ExchangedNode<T>* partner_node = extract_ptr(current);
            if (state == EMPTY) {
                uintptr_t waiting_msg = pack(my_node, WAITING);
                if (slot.compare_exchange_strong(current, waiting_msg, std::memory_order_release)) {  
                    while (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start_time) < timeout) {
                        uintptr_t slot_now = slot.load(std::memory_order_acquire);       
                        uintptr_t state_now = extract_state(slot_now);
                        if (state_now == BUSY) {
                            ExchangedNode<T>* partner_node = extract_ptr(slot_now);

                            std::optional<T> partner_data = partner_node->payload;

                            slot.store(EMPTY, std::memory_order_release);   // musimy zmienić na release, żeby zasygnalizować że skończyliśmy

                            return {true, partner_data};
                            // return partner;
                        }
                        std::this_thread::yield();
                    }
                    if (slot.compare_exchange_strong(waiting_msg, EMPTY, std::memory_order_relaxed)) { // nie musimy zmieniać, bo to jest w sytiacji kiedy i tak partner nie przyszedł
                        return {false, std::nullopt};
                        // return nullptr;
                    } else {
                        uintptr_t final_msg = slot.load(std::memory_order_acquire);
                        ExchangedNode<T>* partner_node = extract_ptr(final_msg);

                        std::optional<T> partner_data = partner_node->payload;

                        slot.store(EMPTY, std::memory_order_release); // musimy zmienić na release, żeby zasygnalizować że skończyliśmy

                        return {true, partner_data};

                        // return partner;
                    }
                }
            } else if (state == WAITING) {
                uintptr_t busy_msg = pack(my_node, BUSY);
                if (slot.compare_exchange_strong(current, busy_msg, std::memory_order_acq_rel)) {

                    ExchangedNode<T>* partner_node = extract_ptr(current); 
                    
                    std::optional<T> partner_data = partner_node->payload;

                    // WAŻNE!!!
                    // w sytuacji, w której np: wątek A robiący push wymienił się z innym wątkiem B robiącym pop
                    // a następnie A szybko wrócił z funkcji visit i z funkcji push zanim 
                    // wątek B robiący pop odebrał jego wartość, która jest schowna w pod adresem box'a A w std::optional
                    // zostaniemy z wiszącą referencją, więc musimy zaczekąć, aż A przejmię tą wiadomość
                    while (extract_state(slot.load(std::memory_order_acquire)) != EMPTY) { 
                        std::this_thread::yield();
                    } // dlatego potrzebujmey tego busywaiting, czeka aż partner odbierze przesyłke

                    return {true, partner_data};

                    // return partner_node;
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

    inline static thread_local std::mt19937 generator{std::random_device{}()}; // każdy wątek ma swój generator liczb losowych

public:
    EliminationArray(int _capacity, std::chrono::microseconds _duration) : 
        capacity(_capacity), duration(_duration), exchanger(_capacity) {}
    
    std::pair<bool, std::optional<T>> visit(ExchangedNode<T>* my_node) {
        std::uniform_int_distribution<int> distribution(0, capacity - 1);
        
        int slot = distribution(generator); 
        return exchanger[slot].exchange(my_node, duration);
    }

    // ExchangedNode<T>* visit(ExchangedNode<T>* my_node) {
    //     std::uniform_int_distribution<int> distribution(0, capacity - 1);
        
    //     int slot = distribution(generator); 
    //     return exchanger[slot].exchange(my_node, duration);
    // }
};

template<typename T>
class LockFreeStack {
private:
    struct Node {
        T data;
        std::shared_ptr<Node> next;

        Node() : next(nullptr) {}
        Node(T val) : data(std::move(val)), next(nullptr) {}
    };

    EliminationArray<T> elimination;

    std::shared_ptr<Node> head{nullptr};

    int capacity;

public: 
    LockFreeStack(int _capacity, std::chrono::microseconds _duration) : capacity(_capacity), elimination(_capacity, _duration) {}

    void push(T val) {
        auto new_node = std::make_shared<Node>(val);
        ExchangedNode<T> box(val);
        while (true) {
            auto current_head = std::atomic_load_explicit(&head, std::memory_order_relaxed);
            new_node->next = current_head;
            if (std::atomic_compare_exchange_weak_explicit(&head, &current_head, new_node, std::memory_order_release, std::memory_order_relaxed)) {
                return;
            } else {
                auto result = elimination.visit(&box);
                bool succes = result.first;
                auto data = result.second;
                if (succes && !data.has_value()) {
                    return;
                }
                // if (result != nullptr && !result->payload.has_value()) {
                //     return;
                // }
            }
        }
    }

    std::optional<T> pop() {
        ExchangedNode<T> box(std::nullopt);
        while (true) {
            auto current_head = std::atomic_load_explicit(&head, std::memory_order_acquire);
            if (current_head == nullptr){ 
                return std::nullopt;
            }
            if (std::atomic_compare_exchange_weak_explicit(&head, &current_head, current_head->next, std::memory_order_relaxed, std::memory_order_acquire)) {
                return std::move(current_head->data);
            } else {
                auto result = elimination.visit(&box);
                bool succes = result.first;
                auto data = result.second;
                if (succes && data.has_value()) {
                    return data;
                }
                // if (result != nullptr && result->payload.has_value()) {
                //     return std::move(result->payload.value());
                // }
                
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

const int TOTAL_OPERATIONS = 1'000'000;

void benchmark_elimination(int num_threads, int capacity, int timeout_us) {
    LockFreeStack<int> stack(capacity, std::chrono::microseconds(timeout_us));

    int ops_per_thread = TOTAL_OPERATIONS / num_threads;

    auto worker = [&stack, ops_per_thread]() {
        for (int i = 0; i < ops_per_thread / 2; i++) {
            stack.push(i);
            stack.pop();
        }
    };

    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> workers;
    for (int i = 0; i < num_threads; i++) {
        workers.emplace_back(worker);
    }
    for(auto& t: workers) {
        t.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    std::cout << "Elimination (Watki: " << std::setw(2) << num_threads 
              << ", Cap: " << std::setw(2) << capacity 
              << ", Time: " << timeout_us << "us) -> " 
              << duration << " ms\n";
}

int main() {
    unsigned int max_threads = std::thread::hardware_concurrency();
    std::cout << "Maksymalna liczba watkow sprzetowych: " << max_threads << "\n\n";

    std::vector<int> thread_counts = {1, 2, 4, 8, static_cast<int>(max_threads)};

    int test_capacity = 10;
    int test_timeout_us = 50;

    std::cout << "Test 1mln operacji\n";
    
    for (int t : thread_counts) {
        // benchmark(t); 
        benchmark_elimination(t, test_capacity, test_timeout_us);
        std::cout << "-\n";
    }

    return 0;
}