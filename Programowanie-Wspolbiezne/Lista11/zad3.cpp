#include <atomic>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

#include <chrono>
#include <vector>
#include <thread>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <random>
#include <cassert>

unsigned const max_hazard_pointers = 100;
struct hazard_pointer {
    std::atomic<std::thread::id> id;
    std::atomic<void*> pointer;
};

hazard_pointer hazard_pointers[max_hazard_pointers];

class hp_owner {
    hazard_pointer* hp;

public:
    hp_owner(hp_owner const&) = delete;
    hp_owner operator=(hp_owner const&) = delete;

    hp_owner() : hp(nullptr) {
        for (unsigned i = 0; i < max_hazard_pointers; ++i) {
            std::thread::id old_id;
            if (hazard_pointers[i].id.compare_exchange_strong(old_id, std::this_thread::get_id())) {
                hp = &hazard_pointers[i];
                break;
            }
        }
        if (!hp) {
            throw std::runtime_error("No hazard pointers available");
        }
    }

    std::atomic<void*>& get_pointer() {
        return hp->pointer;
    }

    ~hp_owner() {
        hp->pointer.store(nullptr);
        hp->id.store(std::thread::id());
    }
};

struct HP_Pair { // w LazyList potrzbujemy zawsze mieć hazard_pointer na akutalny i następny node
    std::atomic<void*>& hp1;
    std::atomic<void*>& hp2;

    ~HP_Pair() { // RAII
        hp1.store(nullptr);
        hp2.store(nullptr);
    }
};

HP_Pair get_hazard_pointer_pair_for_current_thread() {
    thread_local static hp_owner hazard1;
    thread_local static hp_owner hazard2;
    return {hazard1.get_pointer(), hazard2.get_pointer()};
}

template<typename T>
class LazyList {
    struct Node {
        const T data;
        const unsigned key;

        std::atomic<Node*> next{nullptr};

        std::mutex mtx;
        std::atomic<bool> mark{false};

        Node() : data(T()), key(0) {} // dummy node, zawsze będzie przed wszystkimi innymi (zakładamy, ze nasze klucze są >= 1)
        Node(T data_, unsigned key_) : data(std::move(data_)), key(key_) {}

        void lock() {
            mtx.lock();
        }

        void unlock() {
            mtx.unlock();
        }

    };

    std::atomic<Node*> head;

public:

    LazyList() { // dummy node, żeby zawsze coś było na liście
        Node* dummy = new Node();
        head.store(dummy); 
    }

    LazyList(const LazyList&) = delete;
    LazyList& operator=(const LazyList&) = delete;

    LazyList(LazyList&&) = delete;
    LazyList& operator=(LazyList&&) = delete;

    ~LazyList() {
        Node* curr = head.load();
        while (curr) {
            Node* next = curr->next.load();
            delete curr;
            curr = next;
        }
    }

    std::vector<unsigned> debug_get_all_keys() {
        std::vector<unsigned> keys;
        Node* curr = head.load()->next.load(); // Pomijamy dummy node
        while (curr) {
            if (!curr->mark.load()) {
                keys.push_back(curr->key);
            }
            curr = curr->next.load();
        }
        return keys;
    }

    bool add(T item, unsigned key) {
        auto hps = get_hazard_pointer_pair_for_current_thread();
        auto& hp_pred = hps.hp1;
        auto& hp_curr = hps.hp2;

        while (true) {

            Node* pred = head.load();
            Node* curr = nullptr;

            hp_pred.store(pred);  // bezpieczne, bo nigdy nie będziemy usuwać dummy (on nigdy nie będzie marked)
            // ustawiamy hp_curr
            do {
                curr = pred->next.load();           // read
                // if (curr == nullptr) break;
                hp_curr.store(curr);                // set
            } while (pred->next.load() != curr);    // check

            bool again = false; 
            // użycie hazard_ptr jest trochę inne (tutaj jescze nie zajelismy mutexa!)
            // poniższa sytuacja może zajść w add, remove i contains


            // HEAD -> A - > B - > C

            // 1. hp_pred na A i próbuje hp_curr na B, ale zostaje wywłaszczony.
            // 2. Inny wątek usuwa A: oznacza je i przepina head.next na B.
            // 3. Następnie omija A, przechodzi do B i przepina head.next na C.



            // może się zdarzyć taka sytuacja, że jeden wątek przyjdzie, ustawi swoje hp_pred na A
            // nastepnie będzie chciało ustawić hp_curr na B, jednak
            // inny wątek w tym czasie moze zechcieć usunąć A
            // oczywiście, nie zrobi tego, ale ustawi A na marked
            // następnie może usunąc B
            // my nie mamy B w naszym hp_curr (zał, że nikt nie ma)
            // więc B zostanie trwale usunięty ..

            // no i teraz jest problem, bo skoro A zostało zazaczone na marked
            // to już nikt nie zaktualizował jego wskaźnika next na C, next(A) to wciąż B, które jest deleted!!!
            // nasz wątek odwołuje się wyczyszczonego bloku pamięci

            // żeby temu zapobiec musimy sprawdzać, czy pred nie jest marked
            // musimy to zrobć według zasady read-set-check

            while (curr && curr->key < key) { // póki nie spełniliśmy warunków
                pred = curr;
                hp_pred.store(curr);  // bezpieczne, bo na curr mamy już inny hp
                // próbujemy przepiąć się dalej, ustawić nowy hp
                do {
                    curr = pred->next.load();           // read
                    if (curr == nullptr) break;
                    hp_curr.store(curr);                // set
                } while (pred->next.load() != curr);    // check 
                // } while (curr->marked.load());    // check 


                // jeśli pred okazał sie usunięty, to żeby nie dopuścić do sytuacji, 
                // która opisałem wcześniej wystarczy, że zaczniemy od początku
                if (pred->mark.load()) {                // CHECK
                    again = true;
                    break;
                } // raczej mozna sie pozbyc jak bedzimey sprawdzac curr->marked

            }

            if (again) continue; // zaczynym od początku

            {   // bez zmian
                std::lock_guard<Node> lock_pred(*pred);
                {
                    if (curr) {
                        std::lock_guard<Node> lock_curr(*curr);
                        if (validate(pred, curr)) {
                            if (curr->key == key) {
                                return false; // w HP_Pair mamy RAII, więc destruktor sam czyści HP
                            } else {
                                Node* new_node = new Node(item, key);
                                new_node->next.store(curr);
                                pred->next.store(new_node);
                                return true;  // w HP_Pair mamy RAII, więc destruktor sam czyści HP
                            }
                        }
                    } else if (validate(pred)){
                        Node* new_node = new Node(item, key);
                        pred->next.store(new_node);
                        return true;  // w HP_Pair mamy RAII, więc destruktor sam czyści HP
                    }
                }
            }
        }
    }

    std::optional<T> remove(unsigned key) {

        auto hps = get_hazard_pointer_pair_for_current_thread();
        auto& hp_pred = hps.hp1;
        auto& hp_curr = hps.hp2;

        // początek jest analgocizny jak w add

        while (true) {

            Node* pred = head.load();
            Node* curr = nullptr;

            hp_pred.store(pred); 

            do {
                curr = pred->next.load();
                if (curr == nullptr) break;
                hp_curr.store(curr);
            } while (pred->next.load() != curr);

            bool again = false;

            while (curr && curr->key < key) {
                pred = curr;
                hp_pred.store(curr); 

                do {
                    curr = pred->next.load();
                    if (curr == nullptr) break;
                    hp_curr.store(curr);
                } while (pred->next.load() != curr);

                if (pred->mark.load()) {
                    again = true;
                    break;
                }
            }
           
            if (!curr) { // jeśli i tak mamy nullptr to dokopaliśmy się do końca
                return std::nullopt;
            }

            if (again) continue;


            {
                std::unique_lock<Node> lock_pred(*pred); // uniqe_lock żeby móc zrobić unlock wcześniej
                {
                    std::unique_lock<Node> lock_curr(*curr);
                    if (validate(pred, curr)) {
                        if (curr->key != key) {
                            return std::nullopt;
                        } else {
                            curr->mark.store(true);
                            pred->next.store(curr->next.load());
                            T result = std::move(curr->data);

                            // zwalnaimy blokady, żeby nikt bez senus nie czekał kiedy my juz usuwamy wierzchołki

                            hp_curr.store(nullptr); 
                            hp_pred.store(nullptr); 

                            lock_curr.unlock();
                            lock_pred.unlock();

                            if (outstanding_hazard_pointers_for(curr)) { // jak node jest gdzieś hp (u nas już na pewno nie jest!) to doczepiamy
                                reclaim_later(curr);
                            } else { // wpp możemy usunąć
                                delete curr;
                            }
                            delete_nodes_with_no_hazards(); // usuwamy to co możemy
                            return result;
                        }
                    }
                }
            }
        }
    }

    bool contains(unsigned key) {
        auto hps = get_hazard_pointer_pair_for_current_thread(); 
        auto& hp_curr = hps.hp1;
        auto& hp_next = hps.hp2;

        // tutaj analogiczna sytuacja co w add i remove

        while (true) {
            Node* curr = head.load();
            Node* next = nullptr;

            hp_curr.store(curr);

            bool again = false;

            while (curr && curr->key < key) {
                do {
                    next = curr->next.load();
                    if (next == nullptr) break;
                    hp_next.store(next);
                } while(curr->next.load() != next);

                if (curr->mark.load()) {
                    again = true;
                    break;
                }

                curr = next;
                hp_curr.store(next);
            }
            if (!again) return curr && curr->key == key && !curr->mark.load(); 
        }
    }

private:

    bool validate(Node* last) {
        return !last->mark.load() && last->next.load() == nullptr;
    }

    bool validate(Node* pred, Node* curr) { 
        return !pred->mark.load() && !curr->mark.load() && pred->next.load() == curr;
    }

    bool outstanding_hazard_pointers_for(void* target) {
        for (unsigned i = 0; i < max_hazard_pointers; ++i) {
            if (hazard_pointers[i].pointer.load() == target) {
                return true;
            }
        }
        return false;
    }

    template<typename U>
    static void do_delete(void* p) { // static dlatego, że będziemy mieli statyczna liste do usunięcia, metoda nie może brać potajemnie this
        delete static_cast<U*>(p);
    }

    struct data_to_reclaim 
    {
        void* data;
        std::function<void(void*)> deleter;
        data_to_reclaim* next;

        template<typename U>
        data_to_reclaim(U* p) : data(p), deleter(&do_delete<U>), next(nullptr) {}

        ~data_to_reclaim() {
            deleter(data);
        }
    };

    // std::atomic<data_to_reclaim*> nodes_to_reclaim;

    inline thread_local static data_to_reclaim* my_nodes_to_reclaim{nullptr}; // inline, dziwne, ale każdy wątek ma swoją liste, to powinno byc sprawniejsze, ale tez ma swoje wady, bo trzeba czekać na innych aż usuną co trzeba

    void add_to_reclaim_list(data_to_reclaim* node) {
        // node->next = nodes_to_reclaim.load();
        // while (!nodes_to_reclaim.compare_exchange_weak(node->next, node));

        node->next = my_nodes_to_reclaim;
        my_nodes_to_reclaim = node;
    }

    template<typename U>
    void reclaim_later(U* data) {
        add_to_reclaim_list(new data_to_reclaim(data));
    }

    void delete_nodes_with_no_hazards() {
        // data_to_reclaim* current = nodes_to_reclaim.exchange(nullptr);

        data_to_reclaim* current = my_nodes_to_reclaim;
        my_nodes_to_reclaim = nullptr;

        while (current) {
            data_to_reclaim* const next = current->next;
            if (!outstanding_hazard_pointers_for(current->data)) {
                delete current;
            } else {
                add_to_reclaim_list(current);
            }
            current = next;
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

std::atomic<bool> stop_working{false};

void run_sequential_sanity_check() {
    SafePrint() << "[TEST] Sanity check...\n";
    LazyList<int> list;

    assert(list.add(100, 1) == true);
    assert(list.add(200, 2) == true);
    assert(list.add(300, 3) == true);
    assert(list.add(100, 1) == false); // Duplikat

    assert(list.contains(2) == true);
    assert(list.contains(4) == false); // Nie ma tego klucza

    auto val = list.remove(2);
    assert(val.has_value() && val.value() == 200);
    assert(list.contains(2) == false); // Logicznie wyrzucony

    assert(list.remove(2) == std::nullopt); // Fizycznie wyrzucony

    SafePrint() << "[TEST] Sanity check PASSED.\n";
}

void run_concurrent_stress_test() {
    SafePrint() << "[TEST] Running concurrent stress test...\n";
    
    LazyList<int> list;
    std::atomic<bool> stop_flag{false};
    std::atomic<bool> start_signal{false};

    auto producer_task = [&](int id) {
        while (!start_signal.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        std::mt19937 rng(id);
        std::uniform_int_distribution<unsigned> dist(1, 100);

        while (!stop_flag.load(std::memory_order_relaxed)) {
            unsigned key = dist(rng);
            list.add(id * 1000, key); 
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    };

    auto consumer_task = [&](int id) {
        while (!start_signal.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        std::mt19937 rng(id + 100);
        std::uniform_int_distribution<unsigned> dist(1, 100);

        while (!stop_flag.load(std::memory_order_relaxed)) {
            unsigned key = dist(rng);
            list.remove(key);
            std::this_thread::sleep_for(std::chrono::microseconds(15));
        }
    };

    auto reader_task = [&](int id) {
        while (!start_signal.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        std::mt19937 rng(id + 200);
        std::uniform_int_distribution<unsigned> dist(1, 100);

        unsigned found_count = 0;
        while (!stop_flag.load(std::memory_order_relaxed)) {
            unsigned key = dist(rng);
            if (list.contains(key)) {
                found_count++;
            }
        }
    };

    std::vector<std::jthread> threads;

    for (int i = 0; i < 4; ++i) threads.emplace_back(producer_task, i);
    for (int i = 0; i < 4; ++i) threads.emplace_back(consumer_task, i);
    for (int i = 0; i < 4; ++i) threads.emplace_back(reader_task, i);

    start_signal.store(true, std::memory_order_release);

    std::this_thread::sleep_for(std::chrono::seconds(3));

    stop_flag.store(true, std::memory_order_relaxed);

    SafePrint() << "[TEST] Concurrent stress test completed without crashes.\n";
}

void run_deterministic_test() {
    SafePrint() << "[TEST] Running deterministic test...\n";
    LazyList<int> list;

    const int num_threads = 4;
    const int items_per_thread = 1000;
    std::atomic<bool> start_signal{false};

    auto producer = [&](int id) {
        while (!start_signal.load(std::memory_order_acquire)) std::this_thread::yield();
        unsigned start_key = id * items_per_thread + 1;
        unsigned end_key = start_key + items_per_thread - 1;
        
        for (unsigned key = start_key; key <= end_key; ++key) {
            list.add(key * 10, key);
        }
    };

    auto consumer = [&](int id) {
        while (!start_signal.load(std::memory_order_acquire)) std::this_thread::yield();
        unsigned start_key = id * items_per_thread + 1;
        unsigned end_key = start_key + items_per_thread - 1;
        
        for (unsigned key = start_key; key <= end_key; ++key) {
            if (key % 2 == 0) {
                // Kręcimy się, dopóki producent nie doda elementu i my go nie usuniemy
                while (!list.remove(key).has_value()) {
                    std::this_thread::yield();
                }
            }
        }
    };

    {
        std::vector<std::jthread> threads;
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back(producer, i);
            threads.emplace_back(consumer, i);
        }
        start_signal.store(true, std::memory_order_release);
    } // Wychodząc z tego zakresu, jthread automatycznie robi .join() dla wszystkich wątków

    // Weryfikacja stanu struktury (tutaj już jesteśmy jednowątkowo)
    auto final_keys = list.debug_get_all_keys();
    
    unsigned expected_count = (num_threads * items_per_thread) / 2;
    if (final_keys.size() != expected_count) {
        throw std::runtime_error("Deterministic test failed: wrong number of elements!");
    }

    for (size_t i = 0; i < final_keys.size(); ++i) {
        if (final_keys[i] % 2 == 0) {
            throw std::runtime_error("Deterministic test failed: found an even key!");
        }
        if (i > 0 && final_keys[i] <= final_keys[i - 1]) {
            throw std::runtime_error("Deterministic test failed: list is not sorted properly!");
        }
    }

    SafePrint() << "[TEST] Deterministic logic verified. Structural integrity is 100% correct.\n";
}

void run_throughput_benchmark() {
    int sec = 10;
    SafePrint() << std::format("[TEST] Running throughput benchmark ({} seconds)...\n", sec);
    
    LazyList<int> list;
    std::atomic<bool> stop_flag{false};
    std::atomic<bool> start_signal{false};
    std::atomic<uint64_t> total_ops{0};

    auto worker_task = [&](int id) {
        while (!start_signal.load(std::memory_order_acquire)) std::this_thread::yield();

        std::mt19937 rng(id);
        std::uniform_int_distribution<unsigned> dist(1, 1000); // Wąski zakres zwiększa rywalizację
        uint64_t local_ops = 0;

        while (!stop_flag.load(std::memory_order_relaxed)) {
            unsigned op = dist(rng) % 100;
            unsigned key = dist(rng);
            
            if (op < 20) {
                list.add(key * 10, key); // 20% czasu to pisanie
            } else if (op < 40) {
                list.remove(key);        // 20% czasu to usuwanie
            } else {
                list.contains(key);      // 60% czasu to czytanie (wait-free)
            }
            local_ops++;
        }
        total_ops.fetch_add(local_ops, std::memory_order_relaxed);
    };

    std::vector<std::jthread> threads;
    for (int i = 0; i < 8; ++i) threads.emplace_back(worker_task, i); // Mocne obciążenie 8 wątkami


    start_signal.store(true, std::memory_order_release);
    std::this_thread::sleep_for(std::chrono::seconds(sec));
    stop_flag.store(true, std::memory_order_relaxed);
    
    // Niszczenie jthreadów - musimy na nie poczekać przed odczytem total_ops
    threads.clear(); 

    uint64_t ops = total_ops.load();
    SafePrint() << "[TEST] Benchmark finished: " << (ops / sec) << " operations / sec.\n";
}

int main() {
    try {
        run_sequential_sanity_check();
        run_deterministic_test();
        run_throughput_benchmark();
        SafePrint() << "All tests executed successfully. Good job!\n";
    } catch (const std::exception& e) {
        SafePrint() << "Exception caught: " << e.what() << '\n';
    }

    return 0;
}
