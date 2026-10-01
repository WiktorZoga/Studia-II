#include <iostream>
#include <thread>
#include <string>
#include <atomic>
#include <vector>

constexpr uint64_t EBR_INACTIVE = static_cast<uint64_t>(-1);

struct EBRNode {
    void* ptr;
    uint64_t retire_epoch;
    void (*deleter)(void*);
    EBRNode* next;      
};

class EBRDomain {
private:
    std::atomic<uint64_t> global_epoch{0};
    std::vector<std::atomic<uint64_t>> thread_epochs;
    std::atomic<EBRNode*> retire_list_head{nullptr};

public:
    EBRDomain(int max_threads) : thread_epochs(max_threads) {
        for (auto& te : thread_epochs) {
            te.store(EBR_INACTIVE, std::memory_order_relaxed);
        }
    }

    void read_lock(int thread_id) {
        // Wątek ogłasza wejście w najnowszą epokę
        uint64_t e = global_epoch.load(std::memory_order_acquire);
        thread_epochs[thread_id].store(e, std::memory_order_release);
    }

    void read_unlock(int thread_id) {
        thread_epochs[thread_id].store(EBR_INACTIVE, std::memory_order_release);
    }

    template<typename T>
    void retire(T* old_ptr) {
        uint64_t current_epoch = global_epoch.load(std::memory_order_acquire);
        
        EBRNode* node = new EBRNode{
            old_ptr, current_epoch, 
            [](void* p){ delete static_cast<T*>(p); }, nullptr
        };
        
        // Lock-free push (Stos Treibera)
        EBRNode* expected = retire_list_head.load(std::memory_order_relaxed);
        do {
            node->next = expected;
        } while (!retire_list_head.compare_exchange_weak(expected, node, 
                 std::memory_order_release, std::memory_order_relaxed));
        
        reclaim();
    }

private:
    void reclaim() {
        uint64_t current_epoch = global_epoch.load(std::memory_order_acquire);
        bool can_advance = true;
        uint64_t min_active_epoch = current_epoch;
        
        // Określamy minimalną epokę wśród działających wątków
        for (const auto& te : thread_epochs) {
            uint64_t t_epoch = te.load(std::memory_order_acquire);
            if (t_epoch != EBR_INACTIVE) {
                if (t_epoch < min_active_epoch) min_active_epoch = t_epoch;
                if (t_epoch != current_epoch) can_advance = false;
            }
        }

        // Awansowanie globalnej epoki, jeśli wszyscy "dogonili"
        if (can_advance) {
            global_epoch.compare_exchange_strong(current_epoch, current_epoch + 1, std::memory_order_acq_rel);
            current_epoch++; // Zaktualizuj do nowej wartości na potrzeby zwalniania
        }

        EBRNode* current_list = retire_list_head.exchange(nullptr, std::memory_order_acquire);
        EBRNode* to_delete = nullptr;
        EBRNode* to_keep = nullptr;
        
        // Node można usunąć tylko wtedy, gdy żaden wątek nie operuje w epoce jego usunięcia lub starszej
        while (current_list != nullptr) {
            EBRNode* node = current_list;
            current_list = current_list->next;
            
            if (node->retire_epoch < min_active_epoch) {
                node->next = to_delete;
                to_delete = node;
            } else {
                node->next = to_keep;
                to_keep = node;
            }
        }
        
        // Fizyczne zwolnienie pamięci
        while (to_delete) {
            EBRNode* next = to_delete->next;
            to_delete->deleter(to_delete->ptr);
            delete to_delete;
            to_delete = next;
        }
        
        // Przywrócenie zatrzymanych node'ów na stos lock-free
        while (to_keep) {
            EBRNode* node = to_keep;
            to_keep = to_keep->next;
            EBRNode* expected = retire_list_head.load(std::memory_order_relaxed);
            do {
                node->next = expected;
            } while (!retire_list_head.compare_exchange_weak(expected, node, 
                     std::memory_order_release, std::memory_order_relaxed));
        }
    }
};

// Struktura reprezentująca dane, które chcemy chronić przy użyciu RCU
struct SharedConfigEBR {
    int timeout_ms;
    std::string server_url;
};

// Globalny wskaźnik oraz domena RCU
std::atomic<SharedConfigEBR*> global_ebr_config{new SharedConfigEBR{1000, "[http://v1.api.com](http://v1.api.com)"}};
EBRDomain ebr_domain(4); // Domena stworzona dla 4 wątków

// --- WĄTEK CZYTAJĄCY ---
void ebr_reader_thread(int tid) {
    // 1. Zabezpieczenie sekcji krytycznej odczytu
    ebr_domain.read_lock(tid);

    // 2. Bezpieczny odczyt z barierą acquire
    SharedConfigEBR* config = global_ebr_config.load(std::memory_order_acquire);
    
    // Praca na danych - tu wątek modyfikujący nie przeszkodzi nam (bo modyfikuje kopię!)
    std::cout << "[EBR Reader " << tid << "] Odczyt URL: " << config->server_url << "\n";

    // 3. Koniec sekcji krytycznej odczytu
    ebr_domain.read_unlock(tid);
}

// --- WĄTEK MODYFIKUJĄCY ---
void ebr_writer_thread() {
    // 1. Kopia i modyfikacja (tworzymy nową wersję poza sekcją krytyczną)
    SharedConfigEBR* new_config = new SharedConfigEBR{2000, "[http://v2.api.com](http://v2.api.com)"};

    // 2. Publikacja zmian i przechwycenie starego wskaźnika
    SharedConfigEBR* old_config = global_ebr_config.exchange(new_config, std::memory_order_acq_rel);
    
    // 3. Zgłaszamy stary obiekt do usunięcia w tle przez RCU
    ebr_domain.retire(old_config);
    
    std::cout << "[EBR Writer] Konfiguracja zaktualizowana atomowo.\n";
}
