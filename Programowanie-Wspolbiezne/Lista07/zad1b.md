# Pula

## Założenia:
    ... 


```cpp
#include <atomic>
#include <vector>
#include <mutex>

template<typename T>
class Pool {
private:
    struct alignas(64) Node { 
        T data;
        std::atomic<size_t> sequence;
    };
    std::vector<Node> buff;
    std::atomic<size_t> pos_put{0}, pos_get{0};
    size_t const size;

public:
    Pool(size_t size_) : size(size_), buff(size_) {
        for (size_t i = 0; i < size; i++) {
            buff[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    void put(T val) {
        size_t ticket = pos_put.fetch_add(1, std::memory_order_relaxed);
        Node& node = buff[ticket % size];
        while (true) {
            size_t seq = node.sequence.load(std::memory_order_acquire); // READ_SEQ_PUT
            if (seq == ticket) {
                node.data = std::move(val);
                node.sequence.store(ticket + 1, std::memory_order_release); // WRITE_SEQ_PUT
                return;
            }
            std::this_thread::yield();
        }
    }

    T get() {
        size_t ticket = pos_get.fetch_add(1, std::memory_order_relaxed);
        Node& node = buff[ticket % size];
        while (true) {
            size_t seq = node.sequence.load(std::memory_order_acquire); // READ_SEQ_GET
            if (seq == ticket + 1) {
                T val = std::move(node.data); 
                node.sequence.store(ticket + size, std::memory_order_release); // WRITE_SEQ_GET
                return val;
            }
            std::this_thread::yield();
        }
    }
};
```

## Argumentacja

Zapisy w konstrukutrze mają std::memory_order_relaxed, bo w trakcie konstrukcje nie grożą żadne wyścigi.

Tylko put/get zmienia pos_put/pos_get odpowiednio. Potrzebujemy zagwarantować żeby odczyty i zapisy w node'ach były zsynchorizowane bo pracują na nich wszystkie wątki. Wiele wątków może pobierać pos_put, pos_get, ale tam możemy zastosować std::memory_order_relaxed, bo dla tych zmienych nie potrzebujmy nic więcej poza atomowością. Cała logiki wrzucania i wyciągania z puli opiera sie na kolejności ticketów, ale nas realnie nie intereusje ta kolejność w praktyce, mamy jedynie otrzymywac element jak jakiś jest w środku i zapisywać jak jest miejsce. 

# Dowód

## Przekazanie danych (Producent -> Konsument)

Cel: Zapis danych do node'a przez Producenta posiadającego ticket=X musi mieć miejsce przed odczytem tych danych przez Konsumenta posiadającego ten sam ticket=X.

Dlaczego: Aby Konsument, który wylosował z biletomatu numerek X, otrzymał poprawne, w pełni zapisane dane od Producenta obsługującego ten sam numerek X, a nie "śmieci" z pamięci.

Formalnie: "node.data = std::move(val)" (w put dla ticket = X) -[hb]-> "T val = std::move(node.data)" (w get dla ticket = X)

"node.data = std::move(val)" (w put dla ticket = X) -[sb]-> WRITE_SEQ_PUT -[sw]-> READ_SEQ_GET -[sb]-> "T val = std::move(node.data)" (w get dla ticket = X)

Synchronizacja zachodzi, dla tego wątku, który realnie odczyta tą wartość ticket = X, broni tego if.

## Zwolnienie slotu (Konsument -> Producent w kolejnym cyklu)

Cel: Odczyt i zabranie danych przez Konsumenta posiadającego ticket=X musi mieć miejsce przed nadpisaniem tego samego węzła przez Producenta w kolejnym okrążeniu bufora (czyli Producenta z biletem X + size).

Dlaczego: Ponieważ tablica jest zapętlona (ticket % size), nowy Producent po zrobieniu pełnego okrążenia trafi dokładnie w to samo miejsce w pamięci. Nie może on wgrać tam nowych danych, dopóki poprzedni Konsument bezpiecznie nie zabierze starych.

Formalnie: "T val = std::move(node.data)" (w get dla ticket = X) -[hb]-> "node.data = std::move(val)" (w put dla ticket = X + size)

"T val = std::move(node.data)" (w get dla ticket = X) -[sb]-> WRITE_SEQ_GET -[sw]-> READ_SEQ_PUT -[sb]-> "node.data = std::move(val)" (w put dla ticket = X + size)

Synchronizacja zachodzi, dla tego wątku, który realnie odczytam tą wartość ticket = X + size, broni tego if.


