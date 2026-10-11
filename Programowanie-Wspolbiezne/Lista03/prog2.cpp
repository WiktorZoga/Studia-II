#include <iostream>
#include <future>
#include <vector>
#include <stdexcept>

void process_chunk(int id) {
    if (id == 2) {
        throw std::runtime_error("Read error: Corrupted data in parcel 2!");
    }
    std::cout << "Parcel " << id << " processed successfully.\n";
}

int main() {
    std::cout << "\n--- Program 2: Safe error propagation ---\n";
    std::vector<std::future<void>> futures;
    for (int i = 1; i <= 3; ++i) {
        futures.push_back(std::async(std::launch::async, process_chunk, i));
    }
    std::cout << "Waiting for results...\n";
    // EXERCISE: The code below crashes the application when a task throws an exception.
    // Modify it to catch the exception, skip the corrupted parcel,
    // print the error message and allow the program to finish.
    for (int i = 0; i < 3; ++i) {
        // Added a try...catch block
        // async handles exceptions; any captured exception is propagated when .get() is called
        try {
            futures[i].get();
        } catch (const std::exception& e) {
            std::cout << e.what() << "\n";
        }
    }
    std::cout << "All parcels have been accounted for (Success!).\n";
    return 0;
}
