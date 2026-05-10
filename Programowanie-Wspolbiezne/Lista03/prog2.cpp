#include <iostream>
#include <future>
#include <vector>
#include <stdexcept>

void process_chunk(int id) {
    if (id == 2) {
        throw std::runtime_error("Blad odczytu: Uszkodzone dane w paczce 2!");
    }
    std::cout << "Paczka " << id << " przetworzona pomyslnie.\n";
}

int main() {
    std::cout << "\n--- Program 2: Bezpieczna propagacja bledow ---\n";
    std::vector<std::future<void>> futures;
    for (int i = 1; i <= 3; ++i) {
        futures.push_back(std::async(std::launch::async, process_chunk, i));
    }
    std::cout << "Oczekiwanie na wyniki...\n";
    // ZADANIE: Kod ponizej wywala cala aplikacje gdy jedno z zadan rzuca wyjatek.
    // Zmodyfikuj go tak, aby przechwycic wyjatek, zignorowac uszkodzona paczke,
    // wypisac tresc bledu na ekran i pozwolic programowi dokonczyc dzialanie.
    for (int i = 0; i < 3; ++i) {
        // dodałem blok try...catch
        // async obsługje wyjątki a ewnetualny złapany wyjątek zostanie przekazy wyżej po wykonaniu .get
        try { 
            futures[i].get();
        } catch (const std::exception& e) {
            std::cout << e.what() << "\n";
        }
    }
    std::cout << "Wszystkie paczki zostaly podsumowane (Sukces!).\n";
    return 0;
}