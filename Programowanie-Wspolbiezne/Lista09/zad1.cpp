#include <atomic>
#include <memory>
#include <optional>
#include <vector>
#include <thread>
#include <stop_token>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <mutex>

// Zwykły shared_ptr, dlatego że u mnie std::atomic<std::shared_ptr> się nie kompiluje

template<typename T>
class Queue {
    struct Node {
        std::shared_ptr<T> data;
        std::shared_ptr<Node> next; 

        Node() : data(nullptr), next(nullptr) {}
        Node(T val) : data(std::make_shared<T>(std::move(val))), next(nullptr) {}
    };

    std::shared_ptr<Node> head; 
    std::shared_ptr<Node> tail; 

private:
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;
    Queue(Queue&&) = delete;
    Queue& operator=(Queue&&) = delete;

public:
    Queue() {
        auto dummy = std::make_shared<Node>();
        std::atomic_store_explicit(&head, dummy, std::memory_order_relaxed);
        std::atomic_store_explicit(&tail, dummy, std::memory_order_relaxed);
    }

    void enqueue(T val) {
        auto new_node = std::make_shared<Node>(val);
        while (true) {
            auto current_tail = std::atomic_load_explicit(&tail, std::memory_order_acquire);
            auto current_tail_next = std::atomic_load_explicit(&current_tail->next, std::memory_order_acquire);
            if (current_tail == std::atomic_load_explicit(&tail, std::memory_order_relaxed)) {
                if (current_tail_next == nullptr) {
                    std::shared_ptr<Node> expected_next = nullptr;
                    if (std::atomic_compare_exchange_weak_explicit(&current_tail->next, &expected_next, new_node, std::memory_order_release, std::memory_order_relaxed)) {
                        std::atomic_compare_exchange_weak_explicit(&tail, &current_tail, new_node, std::memory_order_release, std::memory_order_relaxed);
                        return;
                    }
                } else {
                    std::atomic_compare_exchange_weak_explicit(&tail, &current_tail, current_tail_next, std::memory_order_release, std::memory_order_relaxed);
                }
            }
        }
    }

    std::optional<T> dequeue() {
        while (true) {
            auto current_head = std::atomic_load_explicit(&head, std::memory_order_acquire);
            auto current_tail = std::atomic_load_explicit(&tail, std::memory_order_acquire);
            auto current_head_next = std::atomic_load_explicit(&current_head->next, std::memory_order_acquire);
            if (current_head == std::atomic_load_explicit(&head, std::memory_order_relaxed)) {
                if (current_head == current_tail) {
                    if (current_head_next == nullptr) { 
                        return std::nullopt;
                    }
                    std::atomic_compare_exchange_weak_explicit(&tail, &current_tail, current_head_next, std::memory_order_release, std::memory_order_relaxed);
                } else {
                    T result = *(current_head_next->data);
                    if (std::atomic_compare_exchange_weak_explicit(&head, &current_head, current_head_next, std::memory_order_release, std::memory_order_relaxed)) {
                        return result;
                    }
                }
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

Queue<int> queue;

void producer(std::stop_token stoken, int id) {
    int it = 0;
    while (!stoken.stop_requested()) {
        queue.enqueue(it * 10 + id);
        it++;
        std::this_thread::sleep_for(std::chrono::milliseconds(5)); 
    }
}

void consumer(std::stop_token stoken, int id) {
    while (!stoken.stop_requested()) {
        std::optional<int> res = queue.dequeue();
        if (res == std::nullopt) {
            // SafePrint() << "Consumer " << id << ": Pusty\n";
        } else {
            SafePrint() << "Consumer " << id << ": " << res.value() << "\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int main() {
    std::vector<std::jthread> producers;
    std::vector<std::jthread> consumers;

    for (int i = 0; i < 3; i++) {
        producers.emplace_back(producer, i);
    }

    for (int i = 0; i < 3; i++) {
        consumers.emplace_back(consumer, i);
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

    for (int i = 0; i < 3; i++) {
        producers[i].request_stop(); 
    }

    for (int i = 0; i < 3; i++) {
        consumers[i].request_stop();
    }

    return 0;
}