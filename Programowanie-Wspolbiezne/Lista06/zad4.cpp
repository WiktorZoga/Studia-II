#include <atomic>
#include <optional>
#include <vector>
#include <thread>
#include <numeric>
#include <set>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <mutex>

template<typename T>
class StealingQueue {
private:
    std::vector<T> queue;
    std::atomic<int64_t> top{0}, bottom{0}; // będzie nas interesował porządek top, bottom, będziemy robic modulo
    int64_t const MAX_SIZE;
public:
    StealingQueue(int64_t size): MAX_SIZE(size), queue(size) {}

    StealingQueue(const StealingQueue&) = delete;
    StealingQueue& operator=(const StealingQueue&) = delete;

    StealingQueue(StealingQueue&&) = delete;
    StealingQueue& operator=(StealingQueue&&) = delete;

    void push(T item) {
        // bottom modyfikuje tylko właściciel, więc odczyt lokalny może być relaxed
        int64_t b = bottom.load(std::memory_order_relaxed);
        
        // acquire synchronizuje się z release złodziei. 
        // gwarantuje, że widzimy najświeższy top po udanych kradzieżach.
        int64_t t = top.load(std::memory_order_acquire);
        
        if (b - t >= MAX_SIZE) return; // zabezpieczenie przed przepełnieniem bufora

        queue[b % MAX_SIZE] = std::move(item);
        
        // gwarantuje, że fizyczny zapis danych do tablicy
        // będzie w pełni widoczny ZANIM bottom ulegnie zwiększeniu.
        bottom.store(b + 1, std::memory_order_release);
    }

    std::optional<T> pop() {
        int64_t b = bottom.load(std::memory_order_relaxed) - 1;
        
        // wymuszamy by złodzieje zobaczyli, że "zaklepaliśmy" element
        // ZANIM my sprawdzimy ich pozycję.
        bottom.store(b, std::memory_order_seq_cst);

        // procesor nie ma prawa zamienić miejscami zapisu bottom i odczytu top
        int64_t t = top.load(std::memory_order_seq_cst);

        if (t < b) {
            // Bezpieczna strefa. Złodzieje są daleko, element jest nasz.
            return std::move(queue[b % MAX_SIZE]);
        }

        std::optional<T> result = std::nullopt;
        
        if (t == b) {
            // ostatni element
            if (top.compare_exchange_strong(t, t + 1, 
                                            std::memory_order_seq_cst, 
                                            std::memory_order_relaxed)) {
                // Wygrywamy wyścig
                result = std::move(queue[b % MAX_SIZE]);
            }
            
            // nowy top to t + 1. 
            // kolejka jest teraz pusta, bottom musi go dogonić,
            // aby zapobiec odwróceniu indeksów (bottom < top).
            bottom.store(t + 1, std::memory_order_relaxed);
            return result;
        }

        // t > b: kolejka była pusta 
        bottom.store(t, std::memory_order_relaxed);
        return std::nullopt;
    }

    std::optional<T> steal() {
        // odczyt top, a następnie bottom. 
        // seq_cst blokuje zamianę tych instrukcji miejscami
        int64_t t = top.load(std::memory_order_seq_cst);
        int64_t b = bottom.load(std::memory_order_seq_cst);

        if (t < b) { 
            // nie możemy tu użyć std::move
            // gdyby wątek to zrobił, a potem przegrał w CAS 
            // z innym złodziejem, ten wygrany dostałby mógłby dostał dummy a nie prawdziwy obiekt
            // bo my już przenieślibyśmy jego dane do naszej zmiennej item
            T item = queue[t % MAX_SIZE]; 

            // wygrywamy wyścig i bierzemy skopiowany element
            if (top.compare_exchange_strong(t, t + 1, 
                                            std::memory_order_seq_cst, 
                                            std::memory_order_relaxed)) {
                return item;
            }
        }
        return std::nullopt;
    }
};

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

void test_logic() {
    StealingQueue<int> q(16);
    q.push(1);
    q.push(2);
    q.push(3);

    auto val1 = q.pop();   // LIFO: powinno być 3
    auto val2 = q.steal(); // FIFO: powinno być 1

    if (val1 == 3 && val2 == 1) {
        SafePrint() << "Prosty test: OK\n";
    } else {
        SafePrint() << "Prosty test: BŁĄD\n";
    }
}

void stress_test() {
    const int num_items = 100000;
    const int num_thieves = 4;
    StealingQueue<int> q(num_items * 2);
    
    std::atomic<int> items_processed{0};
    std::atomic<long long> sum_processed{0};
    std::atomic<bool> done{false};

    std::vector<std::jthread> thieves;
    for (int i = 0; i < num_thieves; ++i) {
        thieves.emplace_back([&]() {
            while (!done || items_processed < num_items) {
                if (auto item = q.steal()) {
                    items_processed++;
                    sum_processed += *item;
                }
                std::this_thread::yield(); 
            }
        });
    }

    long long expected_sum = 0;
    for (int i = 1; i <= num_items; ++i) {
        q.push(i);
        expected_sum += i;
        
        if (i % 10 == 0) {
            if (auto item = q.pop()) {
                items_processed++;
                sum_processed += *item;
            }
        }
    }

    done = true;
    thieves.clear();

    SafePrint() << "Wyniki Stress Testu\n";
    SafePrint() << "Wysłano elementów: " << num_items << "\n";
    SafePrint() << "Przetworzono:      " << items_processed << "\n";
    SafePrint() << "Suma oczekiwana:   " << expected_sum << "\n";
    SafePrint() << "Suma uzyskana:     " << sum_processed << "\n";

    if (sum_processed == expected_sum) {
        SafePrint() << "STATUS: TEST ZALICZONY (Brak wyścigów danych)\n";
    } else {
        SafePrint() << "STATUS: BŁĄD! Sumy się nie zgadzają!\n";
    }
}

void test_conflict() {
    StealingQueue<int> q(1024);
    std::atomic<int> success_count{0};

    for (int i = 0; i < 50000; ++i) {
        q.push(99); 

        std::jthread t([&]() {
            if (q.steal()) success_count++;
        });

        if (q.pop()) success_count++;
        
        t.join();

        if (success_count != i + 1) {
            SafePrint() << "Błąd w iteracji " << i << "! Element zginął lub został podwojony.\n";
            return;
        }
    }
    SafePrint() << "Test Konfliktu: OK (50k iteracji bez błędów)\n";
}

int main() {
    test_logic();
    test_conflict();
    stress_test();
    return 0;
}