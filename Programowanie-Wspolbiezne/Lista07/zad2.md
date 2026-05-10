# LockFreeExchanger

Służy do wymiany danych pomiędzy dwoma wątkami.

```cpp
#include <atomic>
#include <chrono>
#include <thread>
#include <iostream>
#include <cstdint>
#include <vector>
#include <thread>
#include <cassert>
#include <mutex>

// Wymuszamy wyrównanie obiektów do 4 bajtów, co gwarantuje nam,
// że dwa najmłodsze bity wskaźnika będą zawsze równe 0.
template <typename T>
struct alignas(4) ExchangedNode {
    T payload;
    explicit ExchangedNode(T val) : payload(val) {}
};

template <typename T>
class LockFreeExchanger{
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
            uintptr_t current = slot.load(std::memory_order_relaxed);    // READ_SLOT_1
            uintptr_t state = extract_state(current);
            ExchangedNode<T>* partner_node = extract_ptr(current);
            if (state == EMPTY) {
                // "siadamy przy stole"
                uintptr_t waiting_msg = pack(my_node, WAITING);
                if (slot.compare_exchange_strong(current, waiting_msg, std::memory_order_release)) {  // READ&WRITE_SLOT_2
                    // faktycznien mieliśmy empty
                    // teraz czekamy na to aż ktoś zmieni stan na busy, my daliśmy waiting
                    while (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start_time) < timeout) {
                        uintptr_t slot_now = slot.load(std::memory_order_acquire);        // READ_SLOT_3
                        uintptr_t state_now = extract_state(slot_now);
                        if (state_now == BUSY) {
                            // ktoś zmienił na busy, więc możemy zwrócić jego wskaźnik
                            ExchangedNode<T>*  partner = extract_ptr(slot_now);
                            slot.store(EMPTY, std::memory_order_relaxed);      // WRITE_SLOT_4
                            return partner;
                        }
                        // oddajemy czas, może coś sie zmieni
                        std::this_thread::yield();
                    }
                    // timeout - sprzątamy
                    if (slot.compare_exchange_strong(waiting_msg, EMPTY, std::memory_order_relaxed)) { // READ&WRITE_SLOT_5
                        // nikt nie przyszedł do nas
                        return nullptr;
                    } else { // złapaliśmy kogoś w ostatniej chwili, zwracamy tatmejszy stan [BUSY] i czyścimy
                        uintptr_t final_msg = slot.load(std::memory_order_acquire);          // READ_SLOT_6
                        ExchangedNode<T>*  partner = extract_ptr(final_msg);
                        slot.store(EMPTY, std::memory_order_relaxed);          // WRITE_SLOT_7
                        return partner;
                    }
                }
            } else if (state == WAITING) {
                // ktoś już na nas czekał
                uintptr_t busy_msg = pack(my_node, BUSY);
                if (slot.compare_exchange_strong(current, busy_msg, std::memory_order_acq_rel)) { // READ&WRITE_SLOT_8
                    // current zawierało adres partnera, bo extract_ptr(current) to zrobiło wcześniej
                    return partner_node;
                }
            } else if (state == BUSY) {
                std::this_thread::yield(); 
            }
        }
    }
};
```

## Argumentacja

Zmienna atomowa `slot` nie przechowuje bezpośrednio wymieninych danych, a jeydnie wskaźnik do nich oraz stan.
Musimy zapobiec odczytaniu niezainicjowanej pamięci, musimy zagwarantować, że zapis danych zakończy się, zanim inny wątek odczyta wskaźnik ze zmiennej `slot`.

Gdy wątek W1 zmieni stan z `EMPTY` na `WAITING` - READ&WRITE_SLOT_2, używa `release`, aby zagwarantować, że zmiany na danych przez W1 zostały wykonane zanim ustawi `WAITING`. (W1 mógł zmieniać payload zanim wykonał metode exchange na naszym LockFreeExchanger'rze!!!)

Gdy przychodzi wątek W2 i widzi `WAITING`, wykonuje zmianę na `BUSY` - READ&WRITE_SLOT_8. Używa tam `acq_rel`, aby:

* `release` zagwaranotwał publikacje danych przez W2 dla W1
* `acquire` zsychronizowało się z publikacja dancyh przez W1 i pozowiło wziąć dane przez niego opublikowane. 


Następnie W1 czeka w pętli while na zmianę stanu na `BUSY`. Odczyty - READ_SLOT_3 oraz READ_SLOT_6 używają `acquire`. Gwarantuje to synchornizacje z `release` wykonanym przez W2, dane od W2 są gotowe do wzięcia dla W1. 

Sprzątanie przez W1, tzn ustawianie stanu `EMPTY` może zostać wykonane w porządku `relaxed`. Ta zmiana nie wiąże się z przekazywaniem żadnych nowych danych. Analogicznie pobieranie `current`. 


# Dowód

## Przekazanie danych W1 -> W2

Cel: W1: zapis payload -[hb]-> W2: odczyt payload

W1: utworzenie node i zapis payloadu -[sb]-> READ&WRITE_SLOT_2 (release) -[sw]-> READ&WRITE_SLOT_8 (część acquire z acq_rel) -[sb]-> W2: odczyt payloadu ze zwróconego wskaźnika

## Przekazanie danych W2 -> W1

Cel: W2: zapis payloadu -[hb]-> W1: odczyt payloadu

W2: utworzenie node i zapis payloadu -[sb]-> READ&WRITE_SLOT_8 (część release z acq_rel) -[sw]-> READ_SLOT_3 lub READ_SLOT_6 (acquire) -[sb]-> W1: odczyt payloadu ze zwróconego wskaźnika