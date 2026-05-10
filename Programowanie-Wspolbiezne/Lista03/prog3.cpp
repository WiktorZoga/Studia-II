#include <iostream>
#include <future>
#include <thread>
#include <vector>
#include <chrono>
int load_config() {
    std::cout << "[SYSTEM] Ladowanie glownego pliku konfiguracyjnego...\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 42;
}
int main() {
    std::cout << "\n--- Program 3: Oczekiwanie na wspólny zasob ---\n";
    // Tworzymy asynchroniczne zadanie ladowania pliku

    // std::future<int> config_fut = std::async(std::launch::async, load_config);
    std::shared_future<int> config_fut = std::async(std::launch::async, load_config).share(); 
    // wystarczyło linijkę wyżej zastosować shared_future zamiast zwykłego future
    // bez .share() zaszłaby niejawna konwersja

    std::vector<std::thread> algorithms;
    // ZADANIE: Przekazywanie referencji do zwyklego std::future prowadzi do bledu
    // gdy wiele watkow wola get(). Zmien ponizszy kod wykorzystujac std::shared_future.
    for (int i = 1; i <= 3; ++i) {
        // algorithms.push_back(std::thread([&config_fut, i] () {
        algorithms.push_back(std::thread([config_fut, i] () { // zamieniłem referencje na kopie, teraz wykonanie .get() będzie bezpieczniejsze, gdyby z jakiegoś powodu główny wątek zniszczyłby config_fut to zostalibyśmy z wiszącą referencja
        try {
            // To wywolanie spowoduje wyjatek po pierwszym odczycie!
            int config_version = config_fut.get();
            std::cout << "Algorytm " << i << " rozpoczyna prace z w_config " <<
            config_version << "\n";
        } catch (const std::exception &e) {
            std::cout << "Wyjatek w algorytmie " << i << ": " << e.what() << "\n";
        }
        }));
    }
    for (auto &a: algorithms) {
        a.join();
    }
    std::cout << "System dziala.\n";
    return 0;
}