#include <iostream>
#include <vector>
#include <future>
#include <chrono>
#include <thread>
void heavy_processing(int id) {
    std::cout << "[" << id << "] Rozpoczynam analize paczki...\n";
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Symulacja ciężkiej pracy
    std::cout << "[" << id << "] Koniec analizy.\n";
}
int main() {
    std::cout << "--- Program 4: Zlecanie równoległych zadan ---\n";
    auto start = std::chrono::steady_clock::now();
    std::vector<std::future<void>> futures; // tego nie było
    for (int i = 1; i <= 3; ++i) {
        // ZADANIE: Dlaczego ten kod działa sekwencyjnie? Napraw to!
        // std::async(std::launch::async, heavy_processing, i) // tak było
        futures.emplace_back(std::async(std::launch::async, heavy_processing, i)); // tego nie było
    }
    // dodałem tą pętle
    for (auto& f: futures) { // & bo std::future nie można kopiować
        f.get();
    }

    // problem wynikał z tego powodu, ze wszystkie async'ki zwracały futresy z polityką std::launch::async
    // ale nic tego nie przechwytywało, tzn że mieliśmy wiszącą r-wartość std::future<void>, dla której był wykonywane destruktor,
    // w destruktorze std::future jeśli wynik nie został jescze odebrany to ma zostać wtedy odebrany
    // więc w k każdym wykonaniu petli czekalismy na wynik
    // dodając to do wektora a dopiero poźniej zbierając wyniki dajemy szasnce na asynchronicze wykonanie na wielu wątakch

    std::cout << "Wszystkie zadania wyslane (Główny watek idzie dalej)!\n";
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "Calkowity czas: " << diff.count() << " s (a powinno byc ~1s)\n";
    return 0;
}