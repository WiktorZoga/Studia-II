# Kolejka dwuwątkowa

## Założenia:
    jednocześnie co najwyżej jeden wątek może wykonywać operację push oraz co najwyżej jeden może wykonywać pop w wersji wait-free, lock-free. 


```cpp
#include <array>
#include <atomic>
#include <optional>
#include <thread>
#include <mutex>

template<typename T>
class Queue {
private:
    std::vector<T> buff;
    std::atomic<size_t> head{0}, tail{0};
    size_t const size;
public:

    Queue(size_t size_) : size(size_ + 1), buff(size) {}

    bool push(const T& val) {
        size_t current_head = head.load(std::memory_order_acquire); // READ_HEAD_push
        size_t current_tail = tail.load(std::memory_order_relaxed); // READ_TAIL_push
        size_t new_tail = next(current_tail);

        if (new_tail == current_head) {
            return false;
        }

        buff[current_tail] = val;
        tail.store(new_tail, std::memory_order_release);  // WRITE_TAIL_push

        return true;
    }

    std::optional<T> pop() {
        size_t current_head = head.load(std::memory_order_relaxed); // READ_HEAD_pop
        size_t current_tail = tail.load(std::memory_order_acquire); // READ_TAIL_pop

        if (current_head == current_tail) {
            return std::nullopt;
        }

        std::optional<T> opt{buff[current_head]};
        size_t new_head = next(current_head);
        head.store(new_head, std::memory_order_release);   // WRITE_HEAD_pop
        return opt;
    }

private:
    inline size_t next(size_t it) const {
        return (it + 1) % size;
    }
}
```

## Argumentacja

Tylko push modyfikuje tail i tylko pop modyfikuje head, ale oba wątki biorą dostęp do obu końców kolejki.

W push potrzebujemy zagwaranować, że zapis danych do kolejki nastąpi zanim gdziekolwiek zostanie odczytana zmiana tail'a, potrzebujemy tam synchronizacji (SW), dlatego najlżeszym pożądkiem będzie tam std::memory_order_release.

Analogicznie w pop i usuwaniu wartości: std::memory_order_release.

Jako, że tylko jeden wątek na raz robi push, to wie on, że nikt mu tego nie zepsuje, tzn może zrobić to najtańszym kosztem, nie musi synchronizować się sam ze sobą. Dlatego, że czytaniu taila nie musimy mieć SW i możemy sobie pozwolić na std::memory_order_relaxed. Analogicznie dla pop i czytania head. 

Sytuacja zmienia się dla czytania head w push i taila w pop. Nasz wątek tylko czyta, a drugi zmienia (robiąc symetryczną operacje) to co my czytamy. Aby nie mieć problemów musimy zagwarantować (SW), a więc wystarczy std::memory_order_acquire.

## Dowód

Cel: Zapis danych do bufora przez Producenta musi mieć miejsce przed odczytem tych danych przez Konsumenta.

Dlaczego: Żeby Konsument nie przeczytał "śmieci" z pamięci, zanim Producent w ogóle skończył do niej zapisywać.

Formalnie: write: "buff[current_tail] = val" -[hb]-> read "buff[current_head]"

"buff[current_tail] = val" -[sb]-> WRITE_TAIL_push -[sw]-> READ_TAIL_pop -[sb]-> read "buff[current_head]" 

Cel: Usunięcie danych przez Kosumenta musi mieć miejsce przed próbą nadpisania tego miejca przez Producenta.

Dlaczego: Nie chcemy żeby był Konsument przeczytał nie to co miał do przeczytania.

Formalnie: "read buff[current_head]" (tura k) -[hb]->  "buff[current_tail] = val" (tura k+size)

"read buff[current_head]" -[sb]-> WRITE_HEAD_pop -[sw]-> READ_HEAD_push -[sb]-> "buff[current_tail] = val"