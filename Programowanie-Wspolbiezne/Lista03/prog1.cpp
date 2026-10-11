#include <iostream>
#include <future>
#include <chrono>
#include <thread>

bool fetch_parcel() {
    std::cout << "[RESOURCES] Connecting to a very slow database...\n";
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "[RESOURCES] Parcel fetched!\n";
    return true;
}

int main() {
    std::cout << "\n--- Program 1: Lazy loading ---\n";
    auto start = std::chrono::steady_clock::now();
    // EXERCISE: Configure the call to query the database lazily,
    // i.e. only when `needs_parcel` is true and the result is requested.

    // auto parcel_fut = std::async(std::launch::async, fetch_parcel);
    auto parcel_fut = std::async(std::launch::deferred, fetch_parcel); // Changed async to deferred, which uses a lazy execution policy

    bool needs_parcel = false; // A complex calculation may go here
    // to determine the flag's value.
    if (needs_parcel) {
        std::cout << "Processing the parcel...\n";
        parcel_fut.get();
    } else {
        std::cout << "Rejected during quick filtering; the parcel was not needed.\n";
    }
    return 0;
}
