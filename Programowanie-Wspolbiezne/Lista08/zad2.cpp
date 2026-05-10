#include <atomic>
#include <chrono>
#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include <iomanip>
#include <mutex>

class TransactionConsensus {
private:
    std::string payload; // Zwykłe dane, brak atomowości
    double amount;      // Zwykłe dane, brak atomowości
    // Stan: 0 oznacza brak propozycji, >0 oznacza liczbę głosów poparcia
    std::atomic<uint32_t> votes{0};
public:
    // Wątek inicjujący publikuje propozycję transakcji
    void propose(const std::string& p, double a) {
        payload = p;
        amount = a;
        // 1. ZADANIE: Ustaw 'votes' na 1. Użyj najsłabszego,
        // odpowiedniego porządku pamięci, który opublikuje 'payload' i 'amount'.
        // MIEJSCE NA TWÓJ KOD

        votes.store(1, std::memory_order_release); 
        // trzeba zagwarantować, żeby zmiany wykonały sie przed publikacja 'votes'
        // żeby przy odczytaniu votes z tą nową wartością mieć pewnośc że dane się ustawiły już
    }
    // Wątki robocze dodają swoje poparcie (wywoływane współbieżnie)
    void vote_up() {
        // 2. ZADANIE: Zwiększ 'votes' o 1.
        // Zastosuj odpowiednią operację atomową, np. RMW (Read-Modify-Write).
        // MIEJSCE NA TWÓJ KOD

        votes.fetch_add(1, std::memory_order_relaxed); 
        // tutaj może być relaxed, ponieważ ma to być release-sequence
        // te wątki wgl, nie dotykaja danych, mają przenieść synchronizacje dalej

    }

    void vote_up_check() {

        uint32_t expected = votes.load(std::memory_order_acquire); 
        // dlatego, że chcemy sprawdzić payload i amount, potrzebujemy synchornizacji

        if (expected > 0) {
            // tutaj potncjalne odwołanie się do zmiennych, jakaś reguła
            // "Mamy system współbieżny, w którym jeden wyróżniony wątek tworzy (pojedyńczą) transakcję"
            // czyli nie grozi na tutaj data race 
            if (amount > 42.0) {
                // zamiast fetch_add robimy takie CAS, nie możemy rozbić tego na dwie instrukcje relaxed,
                // ponieważ jest wtedy race condition
                while (!votes.compare_exchange_weak(expected, expected + 1, 
                                                        std::memory_order_relaxed, 
                                                        std::memory_order_relaxed));
                // jeśli CAS się nie udał (bo inny wątek był szybszy i podbił licznik), 
                // pętla powtórzy sź z nowym 'expected' 
                
            }
        }

    }

    // Wątek egzekutora próbuje wykonać transakcję
    bool try_execute(std::string& out_p, double& out_a, uint32_t& out_level) const {
        // 3. ZADANIE: Odczytaj 'votes'. Użyj odpowiedniego porządku
        // synchronizującego z publikacją.
        // MIEJSCE NA TWÓJ KOD
        uint32_t level = votes.load(std::memory_order_acquire);
        // ostatni element release-sequence
        if (level > 0) {
            out_p = payload;
            out_a = amount;
            out_level = level;
            return true;
        }
        return false;
    }
};

// Globalny punkt startu programu (do mierzenia relatywnego czasu)
inline const auto start_time = std::chrono::high_resolution_clock::now();

class SafePrint {
public:
    SafePrint() {
        auto now = std::chrono::high_resolution_clock::now();
        auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time).count();
        
        buffer << "[" << std::setw(8) << micros << " us] ";
    }

    ~SafePrint() {
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << buffer.str() << std::flush;
    }

    template<typename T>
    SafePrint& operator<<(const T& msg) {
        buffer << msg;
        return *this;
    }

private:
    static std::mutex mtx;
    std::ostringstream buffer;
};

std::mutex SafePrint::mtx;

TransactionConsensus consensus;
int const NUM_VOTERS = 5;

void initiator_task() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    SafePrint() << "[Inicjator] Nowa propozycja...\n";
    consensus.propose("Zakup XYZ", 100.0);
}

void voter_task(int id) {
    std::string p;
    double a;
    uint32_t l;

    while (!consensus.try_execute(p, a, l)) {
        std::this_thread::yield();
    }

    SafePrint() << "[Glosujacy " << id << "] Zapoznał sie z ofertą '" << p  << "'. Popiera!\n";
              
    consensus.vote_up();
}

void executor_task() {
    std::string p;
    double a;
    uint32_t l;

    uint32_t const REQUIRED_VOTES = 1 + NUM_VOTERS;
    // wszyscy musza sie zgodzić

    while (true) {
        if (consensus.try_execute(p, a, l)) {
            if (l >= REQUIRED_VOTES) {
                SafePrint() << "[EGZEKUTOR] KONSENSUS OSIAGNIETY!\n";
                SafePrint() << "-> Transakcja: " << p << "\n";
                SafePrint() << "-> Kwota:      " << a << " PLN\n";
                SafePrint() << "-> Glosy:      " << l << "/" << REQUIRED_VOTES << "\n";
                break;
            }
        }
        std::this_thread::yield();
    }
}

int main() {
    std::thread executor(executor_task);

    std::vector<std::thread> voters;
    for (int i = 1; i <= NUM_VOTERS; ++i) {
        voters.emplace_back(voter_task, i);
    }

    std::thread initiator(initiator_task);

    initiator.join();
    for (auto& v : voters) {
        v.join();
    }
    executor.join();

    std::cout << "\nTest zakonczony sukcesem.\n";
    return 0;
}