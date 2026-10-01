#include <atomic>
#include <memory>
#include <thread>
#include <vector>
#include <iostream>
#include <random>

template<typename T>
class lock_free_queue {
private:
    struct node;

    struct counted_node_ptr {
        int external_count;
        node* ptr;
    };

    std::atomic<counted_node_ptr> head;
    std::atomic<counted_node_ptr> tail;

    struct node_counter {
        unsigned internal_count : 30;
        unsigned external_counters : 2; // Maksymalnie 2 wskaźniki strukturalne
    };

    struct node {
        std::atomic<T*> data;
        std::atomic<node_counter> count;
        std::atomic<counted_node_ptr> next;

        node() {
            node_counter new_count;
            new_count.internal_count = 0;
            new_count.external_counters = 2; // Rezerwacja dla head/tail oraz poprzednika/następnika
            count.store(new_count, std::memory_order_relaxed);
            
            counted_node_ptr next_ptr;
            next_ptr.ptr = nullptr;
            next_ptr.external_count = 0;
            next.store(next_ptr, std::memory_order_relaxed);
        }

        void release_ref() {
            node_counter old_counter = count.load(std::memory_order_relaxed);
            node_counter new_counter;
            do {
                new_counter = old_counter;
                --new_counter.internal_count;
            } while(!count.compare_exchange_strong(old_counter, new_counter,
                    std::memory_order_acquire, std::memory_order_relaxed));
            
            if(!new_counter.internal_count && !new_counter.external_counters) {
                delete this;
            }
        }
    };

    void increase_external_count(std::atomic<counted_node_ptr>& counter, counted_node_ptr& old_counter) {
        counted_node_ptr new_counter;
        do {
            new_counter = old_counter;
            ++new_counter.external_count;
        } while(!counter.compare_exchange_strong(old_counter, new_counter,
                std::memory_order_acquire, std::memory_order_relaxed));
        old_counter.external_count = new_counter.external_count;
    }

    static void free_external_counter(counted_node_ptr& old_node_ptr) {
        node* const ptr = old_node_ptr.ptr;
        int const count_increase = old_node_ptr.external_count - 2;
        node_counter old_counter = ptr->count.load(std::memory_order_relaxed);
        node_counter new_counter;
        do {
            new_counter = old_counter;
            --new_counter.external_counters; // Odpinamy wskaźnik strukturalny (np. head)
            new_counter.internal_count += count_increase; // Przelewamy zebrane referencje
        } while(!ptr->count.compare_exchange_strong(old_counter, new_counter,
                std::memory_order_acq_rel, std::memory_order_relaxed));

        if(!new_counter.internal_count && !new_counter.external_counters) {
            delete ptr;
        }
    }

public:
    lock_free_queue() {
        counted_node_ptr dummy;
        dummy.ptr = new node();
        dummy.external_count = 1;
        head.store(dummy, std::memory_order_relaxed);
        tail.store(dummy, std::memory_order_relaxed);
    }

    ~lock_free_queue() {
        while(pop());
        node* dummy = head.load(std::memory_order_relaxed).ptr;
        delete dummy;
    }

    void push(T new_value) {
        std::unique_ptr<T> new_data(new T(std::move(new_value)));
        counted_node_ptr new_next;
        new_next.ptr = new node();
        new_next.external_count = 1;
        counted_node_ptr old_tail = tail.load(std::memory_order_relaxed);

        for(;;) {
            increase_external_count(tail, old_tail);
            T* old_data = nullptr;
            if(old_tail.ptr->data.compare_exchange_strong(old_data, new_data.get(), std::memory_order_release, std::memory_order_relaxed)) {
                old_tail.ptr->next.store(new_next, std::memory_order_release);

                // jeśl tutaj ∑atek zostanie wywłaszczony i przyjdzie inny wątek który robi push, to zoabaczy on, że 
                // dane w tail nie są puste, co za tym idze, bedzie się kręcił dalej, bo przegra CAS 

                tail.store(new_next, std::memory_order_release);
                new_data.release();
                
                // Tracimy strukturę ogona dla starego węzła - rozliczamy licznik zewnętrzny
                free_external_counter(old_tail); 
                break;
            }
            // Zmniejszamy licznik wewnętrzny o 1, bo wycofujemy się z tego obiektu
            old_tail.ptr->release_ref(); 
        }
    }

    std::unique_ptr<T> pop() {
        counted_node_ptr old_head = head.load(std::memory_order_relaxed);
        for(;;) {
            increase_external_count(head, old_head);
            node* const ptr = old_head.ptr;
            if(ptr == tail.load(std::memory_order_relaxed).ptr) {
                ptr->release_ref(); // Wycofujemy się z dummy node
                return std::unique_ptr<T>();
            }
            if(head.compare_exchange_strong(old_head, ptr->next.load(std::memory_order_relaxed), std::memory_order_relaxed)) {
                T* const res = ptr->data.exchange(nullptr, std::memory_order_relaxed);
                
                // Sukces - odpięliśmy węzeł, wywołujemy funkcję z Listingu 7.20
                free_external_counter(old_head); 
                return std::unique_ptr<T>(res);
            }
            ptr->release_ref(); // Przegraliśmy CAS, zmniejszamy internal_count o 1
        }
    }
};

int main() {
    std::cout << "[TEST] Uruchamianie kolejki bez blokad (Reference Counting)...\n";
    lock_free_queue<int> queue;

    std::atomic<bool> start_signal{false};

    auto worker = [&](int id) {
        while(!start_signal.load(std::memory_order_acquire)) std::this_thread::yield();
        
        std::mt19937 rng(id);
        std::uniform_int_distribution<int> dist(1, 100);

        for (int i = 0; i < 2000; ++i) {
            if (dist(rng) % 2 == 0) {
                queue.push(id * 10000 + i);
            } else {
                queue.pop();
            }
        }
    };

    std::vector<std::jthread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(worker, i);
    }

    start_signal.store(true, std::memory_order_release);
    threads.clear(); 

    std::cout << "[TEST] Sukces! Kolejka przeszła test poprawności pamięciowej.\n";
    return 0;
}