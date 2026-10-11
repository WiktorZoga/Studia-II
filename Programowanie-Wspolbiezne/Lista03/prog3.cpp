#include <iostream>
#include <future>
#include <thread>
#include <vector>
#include <chrono>
int load_config() {
    std::cout << "[SYSTEM] Loading the main configuration file...\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 42;
}
int main() {
    std::cout << "\n--- Program 3: Waiting for a shared resource ---\n";
    // Create an asynchronous task to load the file

    // std::future<int> config_fut = std::async(std::launch::async, load_config);
    std::shared_future<int> config_fut = std::async(std::launch::async, load_config).share();
    // Using shared_future instead of an ordinary future on the previous line was sufficient
    // Without .share(), an implicit conversion would occur

    std::vector<std::thread> algorithms;
    // EXERCISE: Passing a reference to an ordinary std::future causes an error
    // when multiple threads call get(). Change the code below to use std::shared_future.
    for (int i = 1; i <= 3; ++i) {
        // algorithms.push_back(std::thread([&config_fut, i] () {
        algorithms.push_back(std::thread([config_fut, i] () { // Captured a copy instead of a reference to make .get() safer; otherwise, destroying config_fut in the main thread would leave a dangling reference
        try {
            // Copies of shared_future can read the same result independently.
            int config_version = config_fut.get();
            std::cout << "Algorithm " << i << " starts work with config version " <<
            config_version << "\n";
        } catch (const std::exception &e) {
            std::cout << "Exception in algorithm " << i << ": " << e.what() << "\n";
        }
        }));
    }
    for (auto &a: algorithms) {
        a.join();
    }
    std::cout << "System is running.\n";
    return 0;
}
