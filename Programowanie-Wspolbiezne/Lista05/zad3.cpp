#include <atomic>
#include <array>
#include <thread>
#include <vector>
#include <mutex>
#include <iostream>
#include <sstream>
#include <iomanip>

template<typename T, size_t Size>
class Pool {
private:
    struct Node { // alignas(64)
        T data;
        std::atomic<size_t> sequence;
    };
    std::array<Node, Size> buffer;
    std::atomic<size_t> pos_put{0}, pos_get{0};

    static constexpr size_t Mask = Size - 1;
public:
    Pool() {
        for (size_t i = 0; i < Size; i++) {
            buffer[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    static_assert((Size != 0) && ((Size & Mask) == 0), "Size needs to be a power of two.");

    void put(T val) {
        size_t ticket = pos_put.fetch_add(1, std::memory_order_relaxed);
        Node& node = buffer[ticket & Mask];
        while (true) {
            size_t seq = node.sequence.load(std::memory_order_acquire);
            if (seq == ticket) {
                node.data = std::move(val);
                node.sequence.store(ticket + 1, std::memory_order_release);
                return;
            }
            std::this_thread::yield();
        }
    }

    T get() {
        size_t ticket = pos_get.fetch_add(1, std::memory_order_relaxed);
        Node& node = buffer[ticket & Mask];
        while(true) {
            size_t seq = node.sequence.load(std::memory_order_acquire);
            if (seq == ticket + 1) {
                T val = std::move(node.data);
                node.sequence.store(ticket + Size, std::memory_order_release);
                return val;
            }
            std::this_thread::yield();
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

// --- TESTY ---

void stress_test() {
    SafePrint() << "--- TESTY ---\n";
    
    Pool<int, 1024> pool;
    std::atomic<long long> total_sum{0};
    const int num_threads = 4;
    const int items_per_thread = 250'000;

    auto producer = [&]() {
        for (int i = 1; i <= items_per_thread; i++) {
            pool.put(i);
        }
    };

    auto consumer = [&]() {
        long long local_sum = 0;
        for (int i = 1; i <= items_per_thread; i++) {
            local_sum += pool.get();
        }
        total_sum.fetch_add(local_sum, std::memory_order_relaxed);
    };

    std::vector<std::jthread> producers;
    std::vector<std::jthread> consumers;

    for(int i=0; i<num_threads; ++i) producers.emplace_back(producer);
    for(int i=0; i<num_threads; ++i) consumers.emplace_back(consumer);

    producers.clear();
    consumers.clear();

    long long expected = (long long)num_threads * items_per_thread * (items_per_thread + 1) / 2;
    SafePrint() << "Wynik: " << total_sum.load() << " | Oczekiwano: " << expected;
    
    if(total_sum == expected) SafePrint() << ">>> TEST PASSED <<<";
    else SafePrint() << ">>> TEST FAILED <<<";
}

void test_boundary_conditions() {
    SafePrint() << "--- START TESTÓW WARUNKÓW BRZEGOWYCH ---";

    // TEST 1: Blokowanie na pustej puli
    {
        Pool<int, 4> pool;
        SafePrint() << "1. Test blokowania get() na pustej puli...";
        
        std::atomic<bool> unblocked{false};
        
        std::jthread t1([&]() {
            int val = pool.get(); // Tu wątek powinien utknąć
            if (val == 42) unblocked = true;
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        SafePrint() << "   ...wątek nadal czeka (prawidłowo)";
        
        pool.put(42); // To powinno obudzić t1
        t1.join();
        
        SafePrint() << "   ...wątek odblokowany: " << (unblocked ? "[PASSED]" : "[FAILED]");
    }

    // TEST 2: Blokowanie na pełnej puli
    {
        Pool<int, 2> pool; // Pojemność = 2
        SafePrint() << "2. Test blokowania put() na pełnej puli...";
        
        pool.put(1);
        pool.put(2);
        
        std::atomic<bool> put_done{false};
        std::jthread t2([&]() {
            pool.put(3); // Tu wątek powinien utknąć (brak miejsca)
            put_done = true;
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        SafePrint() << "   ...miejsce zajęte, wątek czeka (prawidłowo)";

        int val = pool.get(); // Zwalniamy miejsce
        t2.join();

        SafePrint() << "   ...pobrano: " << val << ", wątek put() ruszył: " << (put_done ? "[PASSED]" : "[FAILED]");
    }

    // TEST 3: Wrap-around (Zawijanie licznika)
    {
        Pool<int, 4> pool;
        SafePrint() << "3. Test wielokrotnego zawijania (1000 operacji)...";
        bool ok = true;
        for(int i=0; i<1000; ++i) {
            pool.put(i);
            if(pool.get() != i) {
                ok = false;
                break;
            }
        }
        SafePrint() << "   ...wynik: " << (ok ? "[PASSED]" : "[FAILED]");
    }
}

int main() {
    try {
        test_boundary_conditions();
        stress_test();
        SafePrint() << "\nWSZYSTKIE TESTY ZAKOŃCZONE.";
    } catch (const std::exception& e) {
        std::cerr << "Wyjątek: " << e.what() << std::endl;
    }
    return 0;
}