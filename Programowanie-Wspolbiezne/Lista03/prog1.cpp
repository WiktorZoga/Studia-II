#include <iostream>
#include <future>
#include <chrono>
#include <thread>

bool fetch_parcel() {
    std::cout << "[ZASOBY] Nawiazywanie polaczenia z bardzo wolna baza danych...\n";
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "[ZASOBY] Pobrano paczkę!\n";
    return true;
}

int main() {
    std::cout << "\n--- Program 1: Leniwe ladowanie ---\n";
    auto start = std::chrono::steady_clock::now();
    // ZADANIE: Skonfiguruj wywolanie tak, aby baza była odpytywana leniwie,
    // tzn. tylko wtedy gdy zmienna `needs_parcel` będzie false.

    // auto parcel_fut = std::async(std::launch::async, fetch_parcel);
    auto parcel_fut = std::async(std::launch::deferred, fetch_parcel); // tutaj zmiana z async na deferred, deffered zmiania politykę na leniwą

    bool needs_parcel = false; // Tu może być skomplikowane obliczenie
    // ustalające wartość flagi.
    if (needs_parcel) {
        std::cout << "Przetwarzanie paczki...\n";
        parcel_fut.get();
    } else {
        std::cout << "Odrzucono na etapie szybkiej filtracji, paczka nie byla potrzebna.\n";
    }
    return 0;
}