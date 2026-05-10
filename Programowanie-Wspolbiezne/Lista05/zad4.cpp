#include <atomic>
#include <memory>
#include <thread>
#include <vector>
#include <iostream>
#include <sstream>
#include <mutex>
#include <iomanip>

template <typename T>
class Stack { 
private:
    struct Node {
        T data;
        std::shared_ptr<Node> next;
        Node(T val) : data(std::move(val)) {}
    };

    std::shared_ptr<Node> head{nullptr};

public:
    Stack() = default;

    void push(T data) {
        auto new_node = std::make_shared<Node>(std::move(data));
        
        new_node->next = std::atomic_load(&head);

        while (!std::atomic_compare_exchange_weak(&head, &new_node->next, new_node)) {}
    }

    std::shared_ptr<T> pop() {
        std::shared_ptr<Node> old_head = std::atomic_load(&head);
        
        while (old_head && !std::atomic_compare_exchange_weak(&head, &old_head, old_head->next)) {}

        return old_head ? std::make_shared<T>(std::move(old_head->data)) : nullptr;
    }

    bool check_lock_free() const {
        return false;
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


void run_tests() {
    Stack<int> s;

    SafePrint() << "--- TEST 1: Lock-free check ---";
    SafePrint() << "Czy implementacja jest sprzętowo lock-free? " << (s.check_lock_free() ? "TAK" : "NIE");

    SafePrint() << "\n--- TEST 2: Warunki brzegowe ---";
    if (s.pop() == nullptr) SafePrint() << "Pop na pustym stosie: [PASSED]";
    
    s.push(100);
    if (*s.pop() == 100 && s.pop() == nullptr) SafePrint() << "Push/Pop pojedynczy element: [PASSED]";

    SafePrint() << "\n--- TEST 3: Stress Test (Wielowątkowy) ---";
    const int num_threads = 4;
    const int ops_per_thread = 10000;
    std::atomic<int> success_count{0};

    auto task = [&]() {
        for (int i = 0; i < ops_per_thread; ++i) {
            s.push(i);
            if (s.pop() != nullptr) success_count++;
        }
    };

    std::vector<std::jthread> threads;
    for (int i = 0; i < num_threads; ++i) threads.emplace_back(task);
    
    threads.clear();

    SafePrint() << "Wykonano operacji: " << success_count.load();
    if (success_count == num_threads * ops_per_thread) SafePrint() << "Integralność stosu: [PASSED]";
}

int main() {
    run_tests();

    return 0;
}