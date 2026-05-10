#include <atomic>
#include <mutex>
#include <stop_token>
#include <thread>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

class MyConditionVariable {
private:
    std::atomic<uint32_t> counter{0}; // służy do monitorowania zmiany ilości wybudzeń, jako że "tylko rośnie" nie jestemy podani na błedy w tych wybudzeniach
public:

    MyConditionVariable() = default;

    MyConditionVariable(const MyConditionVariable&) = delete;
    MyConditionVariable& operator=(const MyConditionVariable&) = delete;

    MyConditionVariable(MyConditionVariable&&) = delete;
    MyConditionVariable& operator=(MyConditionVariable&&) = delete;

    void wait(std::unique_lock<std::mutex>& lock) { // przydałoby sie dodac jescze memory_order, ale na razie tego nie robimy
        uint32_t current_notifications = counter.load();
        lock.unlock();
        do {
            counter.wait(current_notifications);
        } while (current_notifications == counter.load());
        lock.lock();
    }

    void notify_one() noexcept {
        counter.fetch_add(1); 
        counter.notify_one();
    }

    void notify_all() noexcept {
        counter.fetch_add(1);
        counter.notify_all();
    }
};

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

struct Table {
    std::mutex mtx;
    int dishes{0};
    MyConditionVariable cv;
};

void consumer(std::stop_token stoken, Table& table, int id) {
    while (!stoken.stop_requested()) {
        std::unique_lock<std::mutex> lock(table.mtx);
        while (table.dishes == 0) {
            if (stoken.stop_requested()) return;
            table.cv.wait(lock);
        }
        // jemy
        table.dishes--;

        SafePrint() << "[Konsument " << id << "]: Zjadł, zostało: " << table.dishes << " dań na stole\n";
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void producer(std::stop_token stoken, Table& table, int id) {
    while (!stoken.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        {
            std::lock_guard<std::mutex> lock(table.mtx);
            table.dishes++;
            SafePrint() << "[Producent " << id << "]: Ugotował, jest już: " << table.dishes << " dań na stole\n";
        }
        table.cv.notify_one();
    }
}

int main() {
    Table table;

    std::vector<std::jthread> producers, consumers;
    for (int i = 0; i < 2; i++) {
        producers.emplace_back(producer, std::ref(table), i);
    }

    for (int i = 0; i < 5; i++) {
        consumers.emplace_back(consumer, std::ref(table), i);
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
}