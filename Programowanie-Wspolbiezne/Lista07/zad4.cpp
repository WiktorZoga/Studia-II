#include <atomic>
#include <thread>
#include <iostream>
#include <vector>
#include <cassert>
#include <stop_token>

class TicketLock {
private:
    std::atomic<size_t> next_ticket{0};
    std::atomic<size_t> now_serving{0};

public:
    void lock() {
        size_t my_ticket = next_ticket.fetch_add(1, std::memory_order_relaxed); // nie potrzebujemy nic poza atomowością
        
        while (now_serving.load(std::memory_order_acquire) != my_ticket) {
            // potrzebujemy synchronizacji z unlock oraz zapewnić, że nic z sekcji krytycznej nie wykona się przed zdobyciem dostępu
            std::this_thread::yield(); 
        }
    }

    void unlock() {
        now_serving.fetch_add(1, std::memory_order_release);
        // potrzebujemy zagwaranotować, że wszystko w sekcji krytycznej się wykonało zadnim ją zwolniliśmy
    }
};

// jeśli nastopi overflow, to zostanie wykowane modulo 2^N, o ile nie mamy 2^N wątków nie ma żadnego problemu

TicketLock ticket_lock;

// zmienna krytyczna
size_t shared_counter = 0; 

int const NUM_THREADS = 8;
int const INCREMENTS_PER_THREAD = 100000;

void worker_task(std::stop_token stoken) {

    for (int i = 0; i < INCREMENTS_PER_THREAD && !stoken.stop_requested(); ++i) {
        ticket_lock.lock();

        // sekcja krytyczna
        shared_counter++; 

        ticket_lock.unlock();
    }
}

int main() {

    // Nawet jak dałem wszędzie relaxed, to testy mi przechodziły ...

    std::cout << "Test TicketLocka...\n";
    
    std::vector<std::jthread> threads;
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker_task);
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    for (auto& t : threads) {
        t.request_stop();
    }
    
    size_t expected = NUM_THREADS * INCREMENTS_PER_THREAD;
    std::cout << "Oczekiwana wartosc:  " << expected << "\n";
    std::cout << "Rzeczywista wartosc: " << shared_counter << "\n";
    
    if (shared_counter == expected) {
        std::cout << "WYNIK: SUKCES!\n";
    } else {
        std::cout << "WYNIK: DATA RACE! różnica: "
                  << (expected - shared_counter) << " operacji\n";
    }
    
    return 0;
}