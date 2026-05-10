#include <array>
#include <atomic>
#include <optional>
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <iomanip>

template<typename T, size_t Size>  // Size ma być potęgą dwójki, ale przechowywać będzie o jeden element mniej
class Queue {
private:
    std::array<T, Size> buff;
    std::atomic<size_t> head{0}, tail{0};
public:

    Queue() = default;

    static_assert((Size != 0) && ((Size & (Size - 1)) == 0), "Size needs to be a power of two.");

    bool push(const T& val) {
        size_t current_head = head.load(std::memory_order_acquire);
        size_t current_tail = tail.load(std::memory_order_relaxed);
        size_t new_tail = (current_tail + 1) & (Size - 1);

        if (new_tail == current_head){
            return false;
        }

        buff[current_tail] = val;
        tail.store(new_tail, std::memory_order_release);
        return true;
    }

    std::optional<T> pop() {
        size_t current_head = head.load(std::memory_order_relaxed);
        size_t current_tail = tail.load(std::memory_order_acquire);

        if (current_head == current_tail) {
            return std::nullopt;
        }

        std::optional<T> opt{buff[current_head]};
        size_t new_head = (current_head + 1) & (Size - 1);
        head.store(new_head, std::memory_order_release);
        return opt;
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

void test_edge_cases() {
    SafePrint() << "\n--- TEST WARUNKÓW BRZEGOWYCH ---\n";
    {
        Queue<int, 1> q; // ta kolejka nie zmieści w sobie żadnych danych

        bool push_full = q.push(1);

        SafePrint() << "1. Test `push` na pełnej kolejce: " << (!push_full ? "[PASSED]" : "[FAILED]") << "\n";

        auto pop_empty = q.pop();

        SafePrint() << "2. Test `pop` na pustej kolejce: " << (!pop_empty.has_value() ? "[PASSED]" : "[FAILED]") << "\n";
    }
    {

        Queue<int, 4> q;

        bool push_1 = q.push(1);
        bool push_2 = q.push(2);
        bool push_3 = q.push(3);
        bool push_full = q.push(4);

        bool push_ok = (push_1 && push_2 && push_3) && !push_full;

        auto pop_3 = (q.pop().value() == 1);
        auto pop_2 = (q.pop().value() == 2);
        auto pop_1 = (q.pop().value() == 3);
        auto pop_empty = !(q.pop().has_value());

        bool pop_ok = pop_3 && pop_2 && pop_1 && pop_empty;

        bool passed = (push_ok && pop_ok);

        SafePrint() << "2. Test 4x`push' i 4x`pop` na kolejce o rozmiarze do 3: " << (passed ? "[PASSED]" : "[FAILED]") << "\n";
    }
    {
        Queue<int, 4> q;

        q.push(10);
        q.push(20);
        q.push(30);

        q.pop(); // usuwa 10
        q.pop(); // usuwa 20

        // Teraz tail jest na końcu tablicy. Kolejne operacje push 
        // muszą magicznie przeskoczyć na indeksy 0 i 1.
        bool push_40 = q.push(40);
        bool push_50 = q.push(50);
        bool push_full = q.push(60); // To powinno znowu być odrzucone!

        // Sprawdzamy, czy czytanie też poprawnie "zawinie" head na początek
        auto val_30 = q.pop().value_or(0);
        auto val_40 = q.pop().value_or(0);
        auto val_50 = q.pop().value_or(0);

        bool wrap_ok = push_40 && push_50 && !push_full;
        bool values_ok = (val_30 == 30) && (val_40 == 40) && (val_50 == 50);

        SafePrint() << "3. Test zawijania wskaźników (wrap-around): " 
                    << ((wrap_ok && values_ok) ? "[PASSED]" : "[FAILED]") << "\n";
    }
    {
        Queue<int, 4> q;
        bool marathon_ok = true;

        // Robimy 100 operacji na kolejce o pojemności 3. 
        // Wskaźniki zawiną się wokół tablicy 25 razy
        for (int i = 0; i < 100; ++i) {
            bool pushed = q.push(i);
            auto popped = q.pop();
            
            if (!pushed || popped.value_or(-1) != i) {
                marathon_ok = false;
                break;
            }
        }

        SafePrint() << "4. Test przeplatania (100x push/pop): " 
                    << (marathon_ok ? "[PASSED]" : "[FAILED]") << "\n";
    }
}

void test_multithreaded_stress() {
    SafePrint() << "\n--- TEST WIELOWĄTKOWY ---\n";
    Queue<int, 1024> q;
    const int ELEMENTS = 1'000'000;
    std::atomic<int> errors{0};

    // Producent: push liczby od 1 do 1 000 000
    auto producer = [&]() {
        for (int i = 1; i <= ELEMENTS; ++i) {
            while (!q.push(i)) {
                // Bufor pełny - busy-wait
            }
        }
    };

    // Konsument: odbiera liczby i pilnuje kolejności
    auto consumer = [&]() {
        int expected = 1;
        while (expected <= ELEMENTS) {
            if (auto val = q.pop()) {
                if (val.value() != expected) {
                    errors++; // Znaleźliśmy błąd synchronizacji!
                }
                expected++;
            }
        }
    };

    std::jthread prod(producer);
    std::jthread cons(consumer);

    cons.join();
    prod.join();

    SafePrint() << "1. Test FIFO: " 
                << (errors == 0 ? "[PASSED]" : "[FAILED]") 
                << " (Błędy: " << errors << ")\n";
}

void test_multithreaded_jitter() {
    SafePrint() << "\n--- TEST ZŁOŚLIWEGO PLANISTY ---\n";
    Queue<int, 16> q; 
    const int ELEMENTS = 100'000;
    std::atomic<int> errors{0};

    auto producer = [&]() {
        for (int i = 1; i <= ELEMENTS; ++i) {
            while (!q.push(i)) {
                // Symulujemy, że system operacyjny zabiera nam procesor w najgorszym momencie
                if (i % 5 == 0) {
                    std::this_thread::yield(); 
                }
            }
        }
    };

    auto consumer = [&]() {
        int expected = 1;
        while (expected <= ELEMENTS) {
            if (auto val = q.pop()) {
                if (val.value() != expected) {
                    errors++;
                }
                expected++;
            } else {
                // Symulujemy uśpienie konsumenta, pozwalając producentowi uderzyć w ścianę pełnego bufora
                if (expected % 3 == 0) {
                    std::this_thread::yield();
                }
            }
        }
    };

    std::jthread prod(producer);
    std::jthread cons(consumer);

    cons.join();
    prod.join();

    SafePrint() << "Test Jitter: " 
                << (errors == 0 ? "[PASSED]" : "[FAILED]") 
                << " (Błędy: " << errors << ")\n";
}

int main() {
    
    test_edge_cases();
    
    test_multithreaded_stress();
    test_multithreaded_jitter();
    
    SafePrint() << "\nWszystkie procedury testowe zakonczone!\n";
    return 0;
}