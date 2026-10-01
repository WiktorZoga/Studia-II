#include <atomic>
#include <memory>
#include <optional>
#include <thread>
#include <vector>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <mutex>
#include <string>

template<typename T>
class Queue {
    struct Node {
        std::shared_ptr<T> data;
        std::atomic<Node*> next; 

        Node() : data(nullptr), next(nullptr) {}
        Node(T val) : data(std::make_shared<T>(std::move(val))), next(nullptr) {}
    };

    std::atomic<Node*> head;
    std::atomic<Node*> tail;

    std::atomic<Node*> to_be_deleted{nullptr};
    std::atomic<unsigned> threads_in_queue{0};

public:

    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;
    Queue(Queue&&) = delete;
    Queue& operator=(Queue&&) = delete;

    Queue() {
        Node* dummy = new Node();
        head.store(dummy);
        tail.store(dummy);
    }

    void enqueue(T val) {
        threads_in_queue.fetch_add(1);

        Node* new_node = new Node(val);
        while (true) {
            Node* current_tail = tail.load(); 
            Node* current_tail_next = current_tail->next.load(); 

            if (current_tail == tail.load()) {
                if (current_tail_next == nullptr) {
                    Node* expected = nullptr;
                    if (current_tail->next.compare_exchange_weak(expected, new_node)) {
                        tail.compare_exchange_weak(current_tail, new_node); 
                        threads_in_queue.fetch_sub(1);
                        return;
                    }
                } else {
                    tail.compare_exchange_weak(current_tail, current_tail_next);
                }
            }
        }

    }

    std::shared_ptr<T> dequeue() {
        threads_in_queue.fetch_add(1);

        while (true) {
            Node* current_head = head.load(); 
            Node* current_tail = tail.load(); 
            Node* current_head_next = current_head->next.load();

            if (current_head == head.load()) { 
                if (current_head == current_tail) {
                    if (current_head_next == nullptr) {
                        threads_in_queue.fetch_sub(1);
                        return nullptr;
                    } 
                    tail.compare_exchange_weak(current_tail, current_head_next); 
                } else {
                    std::shared_ptr<T> result = current_head_next->data;
                    if (head.compare_exchange_weak(current_head, current_head_next)) {
                        try_reclaim(current_head);
                        return result;
                    }
                }
            }
        }
    }

    ~Queue() {
        delete_nodes(to_be_deleted.load());
        delete_nodes(head.load());
    }

    private:
        void try_reclaim(Node* old_head) {
            if (threads_in_queue.load() == 1) {
                Node* nodes_to_delete = to_be_deleted.exchange(nullptr);
                if (threads_in_queue.fetch_sub(1) == 1) {
                    delete_nodes(nodes_to_delete);
                } else if (nodes_to_delete) {
                    chain_pending_nodes(nodes_to_delete);
                }
                delete old_head;
            } else {
                chain_pending_node(old_head);
                threads_in_queue.fetch_sub(1);
            }
        }

        static void delete_nodes(Node* nodes) {
            while (nodes) {
                Node* next = nodes->next.load();
                delete nodes;
                nodes = next;
            }
        }

        void chain_pending_node(Node* node) {
            chain_pending_nodes(node, node);
        }

        void chain_pending_nodes(Node* first, Node* last) {
            Node* expected = to_be_deleted.load();
            while (true) {
                last->next.store(expected);
                if (to_be_deleted.compare_exchange_weak(expected, first)) {
                    break; 
                }
            }
        }

        void chain_pending_nodes(Node* nodes) {
            Node* last = nodes;
            while (Node* next = last->next.load()) {
                last = next;
            }
            chain_pending_nodes(nodes, last); 
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

std::atomic<bool> stop_working{false};

std::atomic<unsigned> count_empty{0};

void producer(Queue<int>& q, int id) {
    for (int i = 0; !stop_working.load(std::memory_order_relaxed); ++i) {
        q.enqueue(i);
        SafePrint() << "Producer " << id << ": " << i << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void consumer(Queue<int>& q, int id) {
    for (int i = 0; ; ++i) {
        auto result = q.dequeue();
        std::string data = (result ? std::to_string(*result) : "[empty]");
        SafePrint() << "Consumer " << id << ": " << data << "\n";
        if (!result && stop_working.load(std::memory_order_relaxed)) {
            if (count_empty.fetch_add(1, std::memory_order_relaxed) >= 10) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}

int main() {
    Queue<int> q;

    std::vector<std::jthread> producers;
    std::vector<std::jthread> consumers;

    for (int i = 0; i < 4; i++) {
        producers.emplace_back(producer, std::ref(q), i);
    }

    for (int i = 0; i < 4; i++) {
        consumers.emplace_back(consumer, std::ref(q), i);
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));

    stop_working.store(true, std::memory_order_relaxed); 
    
}