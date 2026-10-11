#include <iostream>
#include <thread>
#include <string>
#include <chrono>
#include <atomic>
#include <vector>

constexpr uint64_t IBR_INACTIVE = static_cast<uint64_t>(-1);

struct IBRNode {
    void* ptr;
    uint64_t birth_epoch;  // Kiedy obiekt powstał
    uint64_t retire_epoch; // Kiedy został wycofany
    void (*deleter)(void*);
    IBRNode* next;
};

// Struktura rezerwacji dla każdego wątku
struct alignas(64) ThreadReservation {
    std::atomic<uint64_t> lower_epoch{IBR_INACTIVE};
    std::atomic<uint64_t> upper_epoch{IBR_INACTIVE};
};

class IBRDomain {
private:
    std::atomic<uint64_t> global_epoch{0};
    std::vector<ThreadReservation> threads;
    std::atomic<IBRNode*> retire_list_head{nullptr};

public:
    IBRDomain(int max_threads) : threads(max_threads) {}

    // Ręczne podbicie zegara w systemie IBR (często wywoływane co N modyfikacji)
    void advance_epoch() {
        global_epoch.fetch_add(1, std::memory_order_acq_rel);
    }

    uint64_t get_current_epoch() const {
        return global_epoch.load(std::memory_order_acquire);
    }

    void read_lock(int thread_id) {
        uint64_t e = global_epoch.load(std::memory_order_acquire);
        threads[thread_id].lower_epoch.store(e, std::memory_order_release);
        threads[thread_id].upper_epoch.store(e, std::memory_order_release);
    }

    // Chroni powolnego czytelnika przed usunięciem nowo utworzonych obiektów, 
    // na które może natrafić podczas długiej iteracji.
    void read_update(int thread_id) {
        uint64_t e = global_epoch.load(std::memory_order_acquire);
        threads[thread_id].upper_epoch.store(e, std::memory_order_release);
    }

    void read_unlock(int thread_id) {
        threads[thread_id].lower_epoch.store(IBR_INACTIVE, std::memory_order_release);
    }

    template<typename T>
    void retire(T* old_ptr, uint64_t birth_epoch) {
        uint64_t retire_epoch = global_epoch.load(std::memory_order_acquire);
        
        IBRNode* node = new IBRNode{
            old_ptr, birth_epoch, retire_epoch, 
            [](void* p){ delete static_cast<T*>(p); }, nullptr
        };
        
        IBRNode* expected = retire_list_head.load(std::memory_order_relaxed);
        do {
            node->next = expected;
        } while (!retire_list_head.compare_exchange_weak(expected, node, 
                 std::memory_order_release, std::memory_order_relaxed));
                 
        reclaim(); 
    }

private:
    void reclaim() {
        // Zrobienie migawki (snapshot) aktywnych wątków dla aktualnego przebiegu reclaim()
        struct TSnap { uint64_t lower; uint64_t upper; };
        std::vector<TSnap> active_threads;
        active_threads.reserve(threads.size());
        
        for (auto& t : threads) {
            uint64_t l = t.lower_epoch.load(std::memory_order_acquire);
            if (l != IBR_INACTIVE) {
                uint64_t u = t.upper_epoch.load(std::memory_order_acquire);
                active_threads.push_back({l, u});
            }
        }

        IBRNode* current_list = retire_list_head.exchange(nullptr, std::memory_order_acquire);
        IBRNode* to_delete = nullptr;
        IBRNode* to_keep = nullptr;
        
        while (current_list != nullptr) {
            IBRNode* node = current_list;
            current_list = current_list->next;
            
            bool overlaps = false;
            // Sprawdzenie głównego warunku krzyżowania się interwałów IBR
            for (const auto& t : active_threads) {
                // Skomplikowana negacja braku nakładania: 
                // Jeżeli NIE JEST Prawdą, że (usunęto przed moim wejściem LUB powstał po moim ostatnim znaku życia)
                if (! (node->retire_epoch < t.lower || node->birth_epoch > t.upper) ) {
                    overlaps = true;
                    break;
                }
            }
            
            if (!overlaps) {
                node->next = to_delete;
                to_delete = node;
            } else {
                node->next = to_keep;
                to_keep = node;
            }
        }
        
        // Zwolnienie bezpiecznej pamięci
        while (to_delete != nullptr) {
            IBRNode* next = to_delete->next;
            to_delete->deleter(to_delete->ptr); 
            delete to_delete;
            to_delete = next;              
        }
        
        // Odłożenie używanych node'ów na listę
        while (to_keep != nullptr) {
            IBRNode* node = to_keep;
            to_keep = to_keep->next;
            IBRNode* expected = retire_list_head.load(std::memory_order_relaxed);
            do {
                node->next = expected;
            } while (!retire_list_head.compare_exchange_weak(expected, node, 
                     std::memory_order_release, std::memory_order_relaxed));
        }
    }
};



// Struktura na potrzeby IBR
struct SharedConfigIBR {
    int max_connections;
    std::string db_host;
    uint64_t birth_epoch; // Zapisany czas narodzin obiektu
};

std::atomic<SharedConfigIBR*> global_ibr_config{new SharedConfigIBR{100, "192.168.1.10", 0}};
IBRDomain ibr_domain(4);

// --- WĄTEK CZYTAJĄCY ---
void ibr_reader_thread(int tid) {
    // 1. Inicjalizacja przedziału rezerwacji (ustawia lower_epoch i upper_epoch)
    ibr_domain.read_lock(tid);

    SharedConfigIBR* config = global_ibr_config.load(std::memory_order_acquire);
    std::cout << "[IBR Reader " << tid << "] Start odczytu: " << config->db_host << "\n";

    // Symulacja długotrwałej operacji na danych (odczyty powolne)...
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 2. Aktualizacja przedziału ochronnego w górę.
    // Przesuwa to zmienną upper_epoch w przód, chroniąc czytelnika przed
    // usunięciem nowo dodanych (i wycofanych) elementów, do których mógłby
    // zaraz dojść idąc dalej po wskaźnikach.
    ibr_domain.read_update(tid);

    // Kolejne, np. równie powolne operacje w ramach tej samej jednostki pracy...
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 3. Całkowite zwolnienie rezerwacji - koniec odczytu (reset lower_epoch)
    ibr_domain.read_unlock(tid);
}

// --- WĄTEK MODYFIKUJĄCY ---
void ibr_writer_thread() {
    uint64_t current_epoch = ibr_domain.get_current_epoch(); 
    SharedConfigIBR* new_config = new SharedConfigIBR{200, "192.168.1.20", current_epoch};
    
    SharedConfigIBR* old_config = global_ibr_config.exchange(new_config, std::memory_order_acq_rel);

    // 1. Pobranie faktycznej epoki systemowej z wnętrza podmienionego obiektu.
    uint64_t birth_epoch = old_config->birth_epoch; 
    
    // 2. Ręczne podbicie globalnego zegara w systemie IBR, tworząc fizyczne
    // rozdzielenie żywotności okien odczytu od okien usunięcia.
    ibr_domain.advance_epoch();
    
    // 3. Wycofanie obiektu z jawnym podaniem momentu jego stworzenia
    ibr_domain.retire(old_config, birth_epoch);
    
    std::cout << "[IBR Writer] Konfiguracja zaktualizowana ze wsparciem przedziałów.\n";
}
