#include <queue>
#include <mutex>
#include <semaphore>
#include <memory>

#include <random>
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>
#include <sstream>

template<typename T>
class BoundedPriorityQueue {
private:
    std::priority_queue<T> pq;
    std::mutex mtx;
    // Semafory do kontroli pojemności (liczba wolnych miejsc i liczba elementów)
    std::counting_semaphore<> free_slots;
    std::counting_semaphore<> available_items;

public:
    explicit BoundedPriorityQueue(int capacity) 
        : free_slots(capacity), available_items(0) {}
    
    void push(const T& value) {
        free_slots.acquire();
        std::lock_guard<std::mutex> lock(mtx);
        try {
            pq.push(value); // tutaj może być wyjątek
            available_items.release();
        }
        catch (...) {
            free_slots.release();
            throw;
        }
    }

    std::shared_ptr<T> wait_and_pop() {
        available_items.acquire();
        std::lock_guard<std::mutex> lock(mtx);
        try {
            auto ptr = std::make_shared<T>(std::move(pq.top()));
            // od tej linijki już nic nie rzuci wyjątku
            pq.pop();
            free_slots.release();
            return ptr; 
        }
        catch (...) {
            available_items.release();
            throw;
        }
    }

    std::shared_ptr<T> try_pop() {
        bool topop = available_items.try_acquire();
        if (!topop) return nullptr;
        std::lock_guard<std::mutex> lock(mtx);
        try {
            auto ptr = std::make_shared<T>(pq.top());
            // od tej linijki już nic nie rzuci wyjątku
            pq.pop();
            free_slots.release();
            return ptr;
        } catch (...) {
            available_items.release();
            throw;
        }
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mtx);
        return pq.empty();
    }
};

class SafePrint {
public:
    SafePrint() = default;

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

void simple_producer(std::stop_token stoken, BoundedPriorityQueue<int>& q, int id) {
    int value = 0;
    while (!stoken.stop_requested()) {
        try {
            q.push(value);
            SafePrint() << "Producent [" << id << "] dodał: " << value << "\n";
            value++;
        } catch (std::exception &e) {
            SafePrint() << e.what() << "\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void simple_consumer(std::stop_token stoken, BoundedPriorityQueue<int>& q, int id) {
    while (!stoken.stop_requested()) {
        auto ptr = q.try_pop();
        if (ptr) SafePrint() << "Konsument [" << id << "] pobrał: " << *ptr << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int main() {
    int const CAPACITY = 5;
    BoundedPriorityQueue<int> pq(CAPACITY);

    std::vector<std::jthread> producers;
    std::vector<std::jthread> consumers;

    std::cout << "START\n";

    for (int i = 0; i < 5; i++) {
        producers.emplace_back(simple_producer, std::ref(pq), i);
        consumers.emplace_back(simple_consumer, std::ref(pq), i);
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

    SafePrint() << "Koniec czasu...\n";

    for (auto& p: producers) p.request_stop();
    for (auto& c: consumers) c.request_stop();

    return 0;
}