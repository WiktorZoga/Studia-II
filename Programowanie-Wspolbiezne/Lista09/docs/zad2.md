```cpp
template<typename T>
class Queue {
    struct Node {
        std::shared_ptr<T> data;
        std::atomic<std::shared_ptr<Node>> next; 

        Node() : data(nullptr), next(nullptr) {}
        Node(T val) : data(std::make_shared<T>(std::move(val))), next(nullptr) {}
    };

    std::atomic<std::shared_ptr<Node>> head;
    std::atomic<std::shared_ptr<Node>> tail;

public:

    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;
    Queue(Queue&&) = delete;
    Queue& operator=(Queue&&) = delete;

    Queue() {
        auto dummy = std::make_shared<Node>();
        head.store(dummy, std::memory_order_relaxed); 
        tail.store(dummy, std::memory_order_relaxed);
        // wszystko dziej sie w konstruktorze i mamy gwarancje na happens-before

        // head i taila wzkazuja na te samą zmienna dummy
    }

    void enqueue(T val) {
        auto new_node = std::make_shared<Node>(val);
        // tworzymy nowy node
        while (true) {
            // kręcimy się aż uda się wstawić nowy element 
            auto current_tail = tail.load(std::memory_order_acquire); 
            // potrzebujemy zagaranować, że odczytywany current_tail_next jest odpowiedni
            auto current_tail_next = current_tail->next.load(std::memory_order_acquire); 
            // relaxed byłoby okej, ale to shared_ptr i potrzbujemy, że counter był odpowiedni

            if (current_tail == tail.load(std::memory_order_relaxed)) { 
                // może zdarzyć się tak, że podczas tego durgiego loada tail nam przeskoczył, więc musimy to sprawdzić
                // tutaj tylko sprawdzamy wartość skaźnika, operujmey na R-value nie potrzebujemy żadnej synchorizacji
                if (current_tail_next == nullptr) {
                    // tutaj sprawdzamy czy faktycznie udało nam się trafić w moment, w ktorym jesteśmy na końcu kolejki
                    std::shared_ptr<Node> expected = nullptr; // expteced musi być L-value
                    if (current_tail->next.compare_exchange_weak(expected, new_node, std::memory_order_release, std::memory_order_relaxed)) {
                        // próbjemy dodać new_node, jeśli się udało to
                        // musimy zagwarantować, że new_node trafi odpwoiednio podczas oczytów w dequeue
                        tail.compare_exchange_weak(current_tail, new_node, std::memory_order_release, std::memory_order_relaxed); 
                        // próbujemy przesunać tail, nie interesuje nas to czy się uda czy nie
                        // jeśli się uda to super, a jeśli nie tzn, że ktoś nas wyreczył
                        // w przypadku sukcesu musimy zagaranotowac poprawność odczytów taila przez inny wątek
                        return;
                    }
                } else {
                    tail.compare_exchange_weak(current_tail, current_tail_next, std::memory_order_release, std::memory_order_relaxed);
                    // tak jak wyżej
                }
            }
        }
    }

    std::optional<T> dequeue() {
        while (true) {
            // kręcimy się, aż uda się usunąć element, albo będziemy pewni, że trafiliśmy w momen, gdy kolejka była pusta
            auto current_head = head.load(std::memory_order_acquire); 
            // potrzebujemy acquire do odczyty current_head_next
            auto current_tail = tail.load(std::memory_order_acquire); 
            // relaxed byłoby ok, ale to shared_ptr, więc potrzebujemy odpowiednio countera, dodatkwo przy akualzacji taila jest istotna
            auto current_head_next = current_head->next.load(std::memory_order_acquire);
            // tutaj acquire jest niezbędny do oczytujemy dane
            if (current_head == head.load(std::memory_order_relaxed)) {
                // tutaj sprawdzamy czy head się nie zmeinił podczas tych kolejnych loadów
                // analogicznie jak wcześniej relaxed 
                if (current_head == current_tail) {
                    // sprawdzamy czy kolejka jest pusta
                    if (current_head_next == nullptr) {
                        // tutaj faktycznie jest pusta
                        return std::nullopt;
                    } 
                    // tutaj okazuje się, że nasz tail jest po porstu nieaktualny
                    tail.compare_exchange_weak(current_tail, current_head_next, std::memory_order_release, std::memory_order_relaxed); 
                    // próbujmey akutaliować go tak jak wcześniej
                } else {
                    // head i tail są rozdzielone
                    T result = *(current_head_next->data);
                    if (head.compare_exchange_weak(current_head, current_head_next, std::memory_order_release, std::memory_order_relaxed)) {
                        // jeśli nasz head jest wciąc akutalny to możemy go przesunać dalej, "nowy dummy", dane są bezpiecznie w current_head_next, który teraz jest wskazywany przed head
                        return result;
                    }
                }
            }
        }
    }
};
```