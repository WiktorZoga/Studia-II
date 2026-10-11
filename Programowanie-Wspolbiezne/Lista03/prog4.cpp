#include <iostream>
#include <vector>
#include <future>
#include <chrono>
#include <thread>
void heavy_processing(int id) {
    std::cout << "[" << id << "] Starting parcel analysis...\n";
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Simulate expensive work
    std::cout << "[" << id << "] Analysis finished.\n";
}
int main() {
    std::cout << "--- Program 4: Submitting parallel tasks ---\n";
    auto start = std::chrono::steady_clock::now();
    std::vector<std::future<void>> futures; // Added here
    for (int i = 1; i <= 3; ++i) {
        // EXERCISE: Why does this code run sequentially? Fix it!
        // std::async(std::launch::async, heavy_processing, i) // Original version
        futures.emplace_back(std::async(std::launch::async, heavy_processing, i)); // Added here
    }
    // Added this loop
    for (auto& f: futures) { // Use & because std::future cannot be copied
        f.get();
    }

    // The problem was that each async call returned a future with the std::launch::async policy
    // but nothing stored it, so the temporary std::future<void> was immediately destroyed.
    // The destructor of such a std::future waits for the result if it has not been retrieved.
    // Thus each loop iteration waited for its result.
    // Storing futures in a vector and collecting results later allows tasks to run asynchronously on multiple threads.

    std::cout << "All tasks submitted (The main thread continues)!\n";
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "Total time: " << diff.count() << " s (expected ~1s)\n";
    return 0;
}
