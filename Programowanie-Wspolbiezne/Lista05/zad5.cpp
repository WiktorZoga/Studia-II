#include <atomic>
#include <chrono>
#include <thread>
#include <iostream>
#include <cstdint>
#include <vector>
#include <thread>
#include <cassert>
#include <sstream>
#include <mutex>
#include <iomanip>

// Wymuszamy wyrównanie obiektów do 4 bajtów, co gwarantuje nam,
// że dwa najmłodsze bity wskaźnika będą zawsze równe 0.
template <typename T>
struct alignas(4) ExchangedNode {
    T payload;
    explicit ExchangedNode(T val) : payload(val) {}
};

template <typename T>
class LockFreeExchanger {
private:
    // Stany maszyny
    static constexpr uintptr_t EMPTY = 0;
    static constexpr uintptr_t WAITING = 1;
    static constexpr uintptr_t BUSY = 2;
    static constexpr uintptr_t STATE_MASK = 3;
    // Atomowa zmienna trzymająca jednocześnie wskaźnik i stan
    std::atomic<uintptr_t> slot{EMPTY};
    // FUNKCJE POMOCNICZE (Tagged Pointers)
    static uintptr_t pack(ExchangedNode<T>* ptr, uintptr_t state) {
        return reinterpret_cast<uintptr_t>(ptr) | state;
    }
    static ExchangedNode<T>* extract_ptr(uintptr_t val) {
        return reinterpret_cast<ExchangedNode<T>*>(val & ~STATE_MASK);
    }
    static uintptr_t extract_state(uintptr_t val) {
        return val & STATE_MASK;
    }
public:
    // Próbuje dokonać wymiany.
    // Zwraca wskaźnik na węzeł od partnera (sukces) lub nullptr (timeout).
    ExchangedNode<T>* exchange(ExchangedNode<T>* my_node, std::chrono::microseconds timeout) {
        auto start_time = std::chrono::steady_clock::now();
        while (true) {
            if (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start_time) >= timeout) {
                return nullptr;
            }
            uintptr_t current = slot.load();
            uintptr_t state = extract_state(current);
            ExchangedNode<T>* partner_node = extract_ptr(current);
            if (state == EMPTY) {
                // "siadamy przy stole"
                uintptr_t waiting_msg = pack(my_node, WAITING);
                if (slot.compare_exchange_strong(current, waiting_msg)) {
                    // faktycznien mieliśmy empty
                    // teraz czekamy na to aż ktoś zmieni stan na busy, my daliśmy waiting
                    while (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start_time) < timeout) {
                        uintptr_t slot_now = slot.load();
                        uintptr_t state_now = extract_state(slot_now);
                        if (state_now == BUSY) {
                            // ktoś zmienił na busy, więc możemy zwrócić jego wskaźnik
                            ExchangedNode<T>*  partner = extract_ptr(slot_now);
                            slot.store(EMPTY);
                            return partner;
                        }
                        // oddajemy czas, może coś sie zmieni
                        std::this_thread::yield();
                    }
                    // timeout - sprzątamy
                    if (slot.compare_exchange_strong(waiting_msg, EMPTY)) {
                        // nikt nie przyszedł do nas
                        return nullptr;
                    } else { // złapaliśmy kogoś w ostatniej chwili, zwracamy tatmejszy stan [BUSY] i czyścimy
                        uintptr_t final_msg = slot.load();
                        ExchangedNode<T>*  partner = extract_ptr(final_msg);
                        slot.store(EMPTY);
                        return partner;
                    }
                }
            } else if (state == WAITING) {
                // ktoś już na nas czekał
                uintptr_t busy_msg = pack(my_node, BUSY);
                if (slot.compare_exchange_strong(current, busy_msg)) {
                    // current zawierało adres partnera, bo extract_ptr(current) to zrobiło wcześniej
                    return partner_node;
                }
            } else if (state == BUSY) {
                std::this_thread::yield(); 
            }
        }
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

void test_exchanger() {
    LockFreeExchanger<int> exchanger;

    // Klasyczna wymiana
    {
        SafePrint() << "TEST 1: Rozpoczynam wymianę pary wątków\n";
        ExchangedNode<int> node1(10);
        ExchangedNode<int> node2(20);

        std::jthread t1([&]() {
            auto res = exchanger.exchange(&node1, std::chrono::milliseconds(500));
            if (res) SafePrint() << "Wątek 1 odebrał: " << res->payload << "\n";
            else SafePrint() << "Wątek 1: TIMEOUT\n";
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Dajemy t1 czas na "ławeczce"

        std::jthread t2([&]() {
            auto res = exchanger.exchange(&node2, std::chrono::milliseconds(500));
            if (res) SafePrint() << "Wątek 2 odebrał: " << res->payload << "\n";
            else SafePrint() << "Wątek 2: TIMEOUT\n";
        });
    }

    // TEST 2: Timeout (nikt nie przychodzi)
    {
        SafePrint() << "\nTEST 2: Test timeoutu\n";
        ExchangedNode<int> node1(999);
        auto start = std::chrono::steady_clock::now();
        
        auto res = exchanger.exchange(&node1, std::chrono::milliseconds(200));
        auto end = std::chrono::steady_clock::now();

        if (res == nullptr) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            SafePrint() << "Wątek poprawnie wyszedł po " << duration << " ms z nullptr.\n";
        }
    }

    // TEST 3: Wielokrotne użycie
    {
        SafePrint() << "\nTEST 3: Test ponownego użycia obiektu\n";
        ExchangedNode<int> n1(1), n2(2), n3(3), n4(4);

        // Pierwsza para
        std::jthread tA([&](){ exchanger.exchange(&n1, std::chrono::milliseconds(100)); });
        std::jthread tB([&](){ exchanger.exchange(&n2, std::chrono::milliseconds(100)); });
        tA.join(); tB.join();

        // Druga para (ten sam exchanger)
        std::jthread tC([&](){ 
            auto res = exchanger.exchange(&n3, std::chrono::milliseconds(100)); 
            if(res) SafePrint() << "Druga wymiana OK, odebrano: " << res->payload << "\n";
        });
        std::jthread tD([&](){ exchanger.exchange(&n4, std::chrono::milliseconds(100)); });
    }
}

int main() {
    test_exchanger();
    return 0;
}