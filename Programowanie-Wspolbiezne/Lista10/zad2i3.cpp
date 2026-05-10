#include <iostream>
#include <vector>
#include <atomic>
#include <string>
#include <thread>
#include <chrono>
#include <random>
#include <functional>
#include <mutex>
#include <iomanip>
#include <iostream>

// Globalny punkt startu programu (do mierzenia relatywnego czasu)
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

unsigned const max_hazard_pointers = 100;
struct hazard_pointer {
    std::atomic<std::thread::id> id;
    std::atomic<void*> pointer;
};

hazard_pointer hazard_pointers[max_hazard_pointers];

class hp_owner {
    hazard_pointer* hp;

public:
    hp_owner(hp_owner const&) = delete;
    hp_owner operator=(hp_owner const&) = delete;

    hp_owner() : hp(nullptr) {
        for (unsigned i = 0; i < max_hazard_pointers; ++i) {
            std::thread::id old_id;
            if (hazard_pointers[i].id.compare_exchange_strong(old_id, std::this_thread::get_id())) {
                hp = &hazard_pointers[i];
                break;
            }
        }
        if (!hp) {
            throw std::runtime_error("No hazard pointers available");
        }
    }

    std::atomic<void*>& get_pointer() {
        return hp->pointer;
    }

    ~hp_owner() {
        hp->pointer.store(nullptr);
        hp->id.store(std::thread::id());
    }
};

struct HP_Pair {
    std::atomic<void*>& hp1;
    std::atomic<void*>& hp2;
};

HP_Pair get_hazard_pointer_pair_for_current_thread() {
    thread_local static hp_owner hazard1;
    thread_local static hp_owner hazard2;
    return {hazard1.get_pointer(), hazard2.get_pointer()};
}

class Hazard_List {
    struct Node {
        std::string data;
        std::atomic<Node*> next;

        Node() : data(""), next(nullptr) {}
        Node(std::string data_) : data(data_), next(nullptr) {}
    };

    std::atomic<Node*> head;
    std::vector<Node*> retire_list;

    bool outstanding_hazard_pointers_for(Node* target) {
        for (unsigned i = 0; i < max_hazard_pointers; ++i) {
            if (hazard_pointers[i].pointer.load(std::memory_order_acquire) == target) {
                return true;
            }
        }
        return false;
    }

public:
    Hazard_List(int size) {
        head.store(new Node("dummy"), std::memory_order_relaxed);
        Node* dummy = head.load(std::memory_order_relaxed);
        
        for (int i = 0; i < size; ++i) {
            Node* new_node = new Node(std::to_string(i));
            new_node->next.store(dummy->next.load(std::memory_order_relaxed), std::memory_order_relaxed);
            dummy->next.store(new_node, std::memory_order_relaxed);
        }
    }

    void read_random(int steps) {
        auto hps = get_hazard_pointer_pair_for_current_thread();
        auto& hp_current = hps.hp1; 
        auto& hp_next = hps.hp2;

        Node* current = head.load(std::memory_order_acquire);
        hp_current.store(current);

        for (int i = 0; i < steps; ++i) {
            Node* next_node;
            do {
                next_node = current->next.load(std::memory_order_acquire);
                if (next_node == nullptr) break;
                hp_next.store(next_node);
            } while(current->next.load(std::memory_order_acquire) != next_node);

            if (next_node == nullptr) break;

            current = next_node;
            hp_current.store(current);
        }

        if (current != head.load(std::memory_order_relaxed)) { 
            SafePrint() << "[Czytelnik " << std::this_thread::get_id() << "] " << current->data << "\n";
        }

        hp_current.store(nullptr);
        hp_next.store(nullptr); 
    }

    void remove_random(int steps) {
        Node* prev = head.load(std::memory_order_acquire);
        Node* current = head.load(std::memory_order_acquire);
        
        for (int i = 0; i < steps; ++i) {
            Node* next_node = current->next.load(std::memory_order_acquire);
            if (next_node == nullptr) break;
            prev = current;
            current = next_node;
        }

        if (prev == current || current == nullptr) return;

        Node* next_node = current->next.load(std::memory_order_acquire);
        prev->next.store(next_node, std::memory_order_release); 

        SafePrint() << "[Pisarz] Odpięto: " << current->data << "\n";

        if (outstanding_hazard_pointers_for(current)) { 
            retire_list.push_back(current);
        } else { 
            SafePrint() << "[Pisarz] Natychmiastowe usunięcie: " << current->data << "\n";
            delete current; 
        }

        scan_retire_list();
    }

    void scan_retire_list() {
        std::vector<Node*> still_in_use;
        for (Node* node: retire_list) {
            if (outstanding_hazard_pointers_for(node)) {
                still_in_use.push_back(node);
            } else {
                SafePrint() << "[Niszczarka] Opóźnione usunięcie: " << node->data << "\n";
                delete node;
            }
        }
        retire_list = std::move(still_in_use);
    }
};

std::atomic<bool> running{true}; 

void reader_thread_task(Hazard_List& list, int reader_id) {
    std::mt19937 rng(std::random_device{}() + reader_id);
    std::uniform_int_distribution<int> dist_steps(1, 40);

    while (running.load(std::memory_order_relaxed)) {
        int steps = dist_steps(rng);
        list.read_random(steps);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void writer_thread_task(Hazard_List& list) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist_steps(1, 40);

    while (running.load(std::memory_order_relaxed)) {
        int steps = dist_steps(rng);
        list.remove_random(steps);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

int main() {
    Hazard_List shared_list(100);

    std::vector<std::thread> readers;
    for (int i = 0; i < 5; ++i) {
        readers.emplace_back(reader_thread_task, std::ref(shared_list), i);
    }

    std::thread writer(writer_thread_task, std::ref(shared_list));

    std::this_thread::sleep_for(std::chrono::seconds(1));

    running.store(false, std::memory_order_relaxed);

    for (auto& r : readers) {
        if (r.joinable()) r.join();
    }
    if (writer.joinable()) writer.join();

    return 0;
}