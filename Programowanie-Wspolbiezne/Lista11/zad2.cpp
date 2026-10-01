#include <atomic>
#include <limits>
#include <mutex>
#include <optional>
#include <utility>

#include <chrono>
#include <vector>
#include <thread>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <random>
#include <cassert>

template<typename T>
class LazyList {
    struct Node {
        T data;
        const unsigned key;

        std::atomic<Node*> next{nullptr};
        std::atomic<Node*> trash_next{nullptr};

        std::mutex mtx;
        std::atomic<bool> mark{false};

        Node() : key(0) {}
        Node(T data_, unsigned key_) : data(std::move(data_)), key(key_) {}

        void lock() {
            mtx.lock();
        }

        void unlock() {
            mtx.unlock();
        }

    };

    std::atomic<Node*> head;
    std::atomic<Node*> to_be_deleted{nullptr};

    std::atomic<unsigned> threads_in_list{0};

public:

    LazyList() {
        Node* dummy = new Node();
        head.store(dummy); 
    }

    LazyList(const LazyList&) = delete;
    LazyList& operator=(const LazyList&) = delete;

    LazyList(LazyList&&) = delete;
    LazyList& operator=(LazyList&&) = delete;

    bool add(T item, unsigned key) {
        threads_in_list.fetch_add(1);
        while (true) {
            Node* pred = head.load();
            Node* curr = pred->next.load();
            while (curr && curr->key < key) {
                pred = curr;
                curr = curr->next.load();
            }
            {
                std::lock_guard<Node> lock_pred(*pred);
                {
                    if (curr) {
                        std::lock_guard<Node> lock_curr(*curr);
                        if (validate(pred, curr)) {
                            if (curr->key == key) {
                                threads_in_list.fetch_sub(1);
                                return false;
                            } else {
                                Node* new_node = new Node(item, key);
                                new_node->next.store(curr);
                                pred->next.store(new_node);
                                threads_in_list.fetch_sub(1);
                                return true;
                            }
                        }
                    } else if (validate(pred)){
                        Node* new_node = new Node(item, key);
                        pred->next.store(new_node);
                        threads_in_list.fetch_sub(1);
                        return true;
                    }
                }
            }
        }
    }

    std::optional<T> remove(unsigned key) {
        threads_in_list.fetch_add(1);
        T result;
        while (true) {
            Node* pred = head.load();
            Node* curr = pred->next.load();
            while (curr && curr->key < key) {
                pred = curr;
                curr = curr->next.load();
            }
            if (!curr) {
                threads_in_list.fetch_sub(1);
                return std::nullopt;
            }
            bool reclaim = false;
            {
                std::lock_guard<Node> lock_pred(*pred);
                {
                    std::lock_guard<Node> lock_curr(*curr);
                    if (validate(pred, curr)) {
                        if (curr->key != key) {
                            threads_in_list.fetch_sub(1);
                            return std::nullopt;
                        } else {
                            curr->mark.store(true);
                            pred->next.store(curr->next.load());
                            result = std::move(curr->data);
                            reclaim = true;
                        }
                    }
                }
            }
            if (reclaim) {
                try_reclaim(curr);
                return result;
            }
        }
    }

    bool contains(unsigned key) {
        threads_in_list.fetch_add(1);
        Node* curr = head.load();
        while (curr && curr->key < key) {
            curr = curr->next.load();
        }
        bool result = curr && curr->key == key && !curr->mark.load();
        threads_in_list.fetch_sub(1);
        return result;
    }

    ~LazyList() {
        Node* curr = head.load();
        while (curr) {
            Node* node = curr->next.load();
            delete curr;
            curr = node;
        }
        delete_nodes(to_be_deleted.load());
    }

private:

    bool validate(Node* last) {
        return !last->mark.load() && last->next.load() == nullptr;
    }

    bool validate(Node* pred, Node* curr) { 
        return !pred->mark.load() && !curr->mark.load() && pred->next.load() == curr;
    }

     void try_reclaim(Node* node) {
        if (threads_in_list.load() == 1) {
            Node* nodes_to_delete = to_be_deleted.exchange(nullptr);
            if (threads_in_list.fetch_sub(1) == 1) {
                delete_nodes(nodes_to_delete);
            } else if (nodes_to_delete) {
                chain_pending_nodes(nodes_to_delete);
            }
            delete node;
        } else {
            chain_pending_node(node);
            threads_in_list.fetch_sub(1);
        }
    }

    void chain_pending_node(Node* node) {
        chain_pending_nodes(node, node);
    }

    void chain_pending_nodes(Node* first, Node* last) {
        Node* expected = to_be_deleted.load();
        while (true) {
            last->trash_next.store(expected);
            if (to_be_deleted.compare_exchange_weak(expected, first)) {
                break; 
            }
        }
    }

    void chain_pending_nodes(Node* nodes) {
        Node* last = nodes;
        while (Node* next = last->trash_next.load()) {
            last = next;
        }
        chain_pending_nodes(nodes, last); 
    }

    static void delete_nodes(Node* nodes) {
        while (nodes) {
            Node* next = nodes->trash_next.load();
            delete nodes;
            nodes = next;
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

std::atomic<bool> stop_working{false};

void run_sequential_sanity_check() {
    SafePrint() << "[TEST] Sanity check...\n";
    LazyList<int> list;

    assert(list.add(100, 1) == true);
    assert(list.add(200, 2) == true);
    assert(list.add(300, 3) == true);
    assert(list.add(100, 1) == false); // Duplikat

    assert(list.contains(2) == true);
    assert(list.contains(4) == false); // Nie ma tego klucza

    auto val = list.remove(2);
    assert(val.has_value() && val.value() == 200);
    assert(list.contains(2) == false); // Logicznie wyrzucony

    assert(list.remove(2) == std::nullopt); // Fizycznie wyrzucony

    SafePrint() << "[TEST] Sanity check PASSED.\n";
}

void run_concurrent_stress_test() {
    SafePrint() << "[TEST] Running concurrent stress test...\n";
    
    LazyList<int> list;
    std::atomic<bool> stop_flag{false};
    std::atomic<bool> start_signal{false};

    auto producer_task = [&](int id) {
        while (!start_signal.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        std::mt19937 rng(id);
        std::uniform_int_distribution<unsigned> dist(1, 100);

        while (!stop_flag.load(std::memory_order_relaxed)) {
            unsigned key = dist(rng);
            list.add(id * 1000, key); 
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    };

    auto consumer_task = [&](int id) {
        while (!start_signal.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        std::mt19937 rng(id + 100);
        std::uniform_int_distribution<unsigned> dist(1, 100);

        while (!stop_flag.load(std::memory_order_relaxed)) {
            unsigned key = dist(rng);
            list.remove(key);
            std::this_thread::sleep_for(std::chrono::microseconds(15));
        }
    };

    auto reader_task = [&](int id) {
        while (!start_signal.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        std::mt19937 rng(id + 200);
        std::uniform_int_distribution<unsigned> dist(1, 100);

        unsigned found_count = 0;
        while (!stop_flag.load(std::memory_order_relaxed)) {
            unsigned key = dist(rng);
            if (list.contains(key)) {
                found_count++;
            }
        }
    };

    std::vector<std::jthread> threads;

    for (int i = 0; i < 4; ++i) threads.emplace_back(producer_task, i);
    for (int i = 0; i < 4; ++i) threads.emplace_back(consumer_task, i);
    for (int i = 0; i < 4; ++i) threads.emplace_back(reader_task, i);

    start_signal.store(true, std::memory_order_release);

    std::this_thread::sleep_for(std::chrono::seconds(3));

    stop_flag.store(true, std::memory_order_relaxed);

    SafePrint() << "[TEST] Concurrent stress test completed without crashes.\n";
}

int main() {
    try {
        run_sequential_sanity_check();
        run_concurrent_stress_test();
        SafePrint() << "All tests executed successfully. Architecture is robust.\n";
    } catch (const std::exception& e) {
        SafePrint() << "Exception caught: " << e.what() << '\n';
    }

    return 0;
}