#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <vector>
#include <random>

int const MAX_BUFOR_SIZE = 700;
int const NUMBER_OF_WORKERS = 13;
int const MENU_SIZE = 100;
int const WORKING_TIME = 5;

std::chrono::steady_clock::time_point closing_time;

std::mutex print_mutex;

template <typename T>
class BoundedBuffer {
private:
    int const MAX_SIZE;
    std::queue<T> bufor;
    mutable std::mutex mtx;

    std::condition_variable not_full;
    std::condition_variable not_empty;

public:
    BoundedBuffer(int max_size): MAX_SIZE(max_size) {}

    BoundedBuffer(const BoundedBuffer& prod) = delete;
    BoundedBuffer(BoundedBuffer&& pord) = delete;
    BoundedBuffer& operator=(const BoundedBuffer&) = delete;
    BoundedBuffer& operator=(BoundedBuffer&&) = delete;

    bool produce(T value) {
        std::unique_lock<std::mutex> lock(mtx);

        bool succes = not_full.wait_until(lock, closing_time, [this]() {
            return bufor.size() < MAX_SIZE;
        });

        if (!succes) {
            return false;
        }

        bufor.push(value);

        not_empty.notify_one();

        return true;
    }

    bool consume(T& out_value) {
        std::unique_lock<std::mutex> lock(mtx);

        bool succes = not_empty.wait_until(lock, closing_time, [this]() {
            return !bufor.empty();
        });

        if (!succes) {
            return false;
        }

        out_value = bufor.front();
        bufor.pop();

        not_full.notify_one();

        return true;
    }

};

BoundedBuffer<int> myBuffer(MAX_BUFOR_SIZE);

void producer() {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(1, MENU_SIZE);

    int value = dist(rng);
    bool free = false;

    while (std::chrono::steady_clock::now() < closing_time) {
        free = myBuffer.produce(value);
        if (free) {
            std::lock_guard<std::mutex> lock(print_mutex);
            std::cout << "Producer: " << std::this_thread::get_id() << " " << value << "\n";
            value = dist(rng);
        }
    }
}

void consumer() {
    while (std::chrono::steady_clock::now() < closing_time) {
        int value = -1;
        if (myBuffer.consume(value)) {
            std::lock_guard<std::mutex> lock(print_mutex);
            std::cout << "Consumer: " << std::this_thread::get_id() << " " << value << "\n";
        }
    }
}

int main() {

    std::cout << "Openning the bar!\n";

    std::vector<std::jthread> producers, consumers;

    closing_time = std::chrono::steady_clock::now() + std::chrono::seconds(WORKING_TIME);

    for (int i = 0; i < NUMBER_OF_WORKERS; i++) {
        producers.push_back(std::jthread(producer));
        consumers.push_back(std::jthread(consumer));
    }

    for (int i = 0; i < NUMBER_OF_WORKERS; i++) {
        if (producers[i].joinable()) producers[i].join();
        if (consumers[i].joinable()) consumers[i].join();
    }

    std::cout << "Closign the bar\n";

    return 0;
}