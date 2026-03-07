#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <random>
#include <chrono>

int const MAX_SIZE = 700;
int const NUMBER_OF_WORKERS = 13;
int const MENU_SIZE = 100;

int const WORKING_TIME = 3;

std::queue<int> bufor;

std::mutex mtx;

std::chrono::steady_clock::time_point closing_time;

bool produce(int value) {
    std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);

    if (!lock.owns_lock()) {
        return false;
    }

    if (bufor.size() < MAX_SIZE) {
        // std::cout << "Producer " << std::this_thread::get_id() << ": " << value << "\n";
        bufor.push(value);
        return true;
    }
    return false;
}

bool consume() {
    std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);

    if (!lock.owns_lock()) {
        return false;
    }

    if (!bufor.empty()) {
        int value = bufor.front();
        // std::cout << "Consumer " << std::this_thread::get_id() << ": " << value << "\n";
        bufor.pop();
        return true;
    }

    return false;
}

void producer() {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> meal_dist(0, MENU_SIZE);

    int current_meal = meal_dist(rng);

    while (std::chrono::steady_clock::now() < closing_time) {
        if (!produce(current_meal)) {
            std::this_thread::yield();
        } else {
            current_meal = meal_dist(rng);
        }
    }
}

void consumer() {
    while (std::chrono::steady_clock::now() < closing_time) {
        if (!consume()) {
            std::this_thread::yield();
        }
    }
}
 
int main() {

    std::cout << "Openning the bar!\n";

    std::vector<std::thread> producers, consumers;

    closing_time = std::chrono::steady_clock::now() + std::chrono::seconds(WORKING_TIME);

    for (int i = 0; i < NUMBER_OF_WORKERS; i++) {
        producers.push_back(std::thread(producer));
        consumers.push_back(std::thread(consumer));
    }

    for (int i = 0; i < NUMBER_OF_WORKERS; i++) {
        producers[i].join();
        consumers[i].join();
    }

    std::cout << "Closing the bar.\n";
    std::cout << "Bufor size and the end: " << bufor.size() << "\n";

    return 0;

}