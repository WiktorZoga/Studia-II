#include <atomic>
#include <thread>
#include <utility>
#include <iostream>
#include <vector>
#include <chrono>

template<typename T>
class SeqLock { // będzie szczególnie wydajny gdy będzie dużo czytających a kopiowanie będzie bardzo szybkie
private:
    std::atomic<T> payload; 
    std::atomic<size_t> seq{0};

public:
    SeqLock() = default;

    // void write(const T& val) {
    //     seq.fetch_add(1, std::memory_order_acquire);  // bariera acquire nie pozwala na przeniesienie zapisu payloadu przed oznaczenie stanu jako nieparzysty (w trakcie modyfikacji)
    //     payload.store(val, std::memory_order_relaxed); 
    //     seq.fetch_add(1, std::memory_order_release); // musimy zapewnić, że dane zostały przeniesione zanim upubliczniliśmy nowy licznik
    // }

    void write(const T& val) {
        size_t s = seq.load(std::memory_order_relaxed);
        seq.store(s + 1, std::memory_order_relaxed);

        std::atomic_thread_fence(std::memory_order_release);

        payload.store(val, std::memory_order_relaxed);
        
        seq.store(s + 2, std::memory_order_release);
    }

    T read() const {
        while (true) {
            size_t current_seq = seq.load(std::memory_order_acquire); 
            if (current_seq % 2 == 0) {
                T result = payload.load(std::memory_order_relaxed);
                
                std::atomic_thread_fence(std::memory_order_acquire);
                
                if (current_seq == seq.load(std::memory_order_relaxed)) { 
                    return result;
                }
            }
            std::this_thread::yield();
        }        
    }

};

// Struktura testowa ma kilka pól żeby spróbować wykryć błędy w zapisie
struct alignas(16) TestData {
    size_t a;
    size_t b;
    size_t c;
};

// Wymuszenie przez kompilator, by upewnić się, że struktura jest bezpieczna dla Seqlocka
// Typ musi być trywialnie kopiowalny, aby w przypadku kolizji jedynym problemem były "wymieszane bajty"
// a nie błedy w alokacji pamięci
static_assert(std::is_trivially_copyable_v<TestData>, "TestData musi byc trivially copyable!");

SeqLock<TestData> global_seqlock;
std::atomic<size_t> fails_detected{0};
std::atomic<size_t> successful_reads{0};

void writer_task(std::stop_token stoken) {
    size_t counter = 0;
    while (!stoken.stop_requested()) {
        counter++;
        global_seqlock.write({counter, counter, counter});
    }
}

void reader_task(std::stop_token stoken) {
    size_t local_success_count = 0;
    while (!stoken.stop_requested()) {
        TestData data = global_seqlock.read();
        if (data.a != data.b || data.b != data.c) { // fail!
            fails_detected.fetch_add(1, std::memory_order_relaxed);
        } else {
            local_success_count++;
        }
    }
    // szybsze i bezpieczniejsze na koniec
    successful_reads.fetch_add(local_success_count, std::memory_order_relaxed);
}

int main() { // jak w pierwszym w load'zie w read damy relaxed to będa błedy!!!
    std::cout << "Test SeqLocka...\n";

    global_seqlock.write({0, 0, 0});

    std::jthread writer(writer_task);

    std::vector<std::jthread> readers;
    for (int i = 0; i < 5; ++i) {
        readers.emplace_back(reader_task);
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

    writer.request_stop();
    for (auto& r : readers) {
        r.request_stop();
    }

    std::cout << "Test zakonczony.\n";
    std::cout << "Udane: " << successful_reads.load() << "\n";
    std::cout << "Bledy: " << fails_detected.load() << "\n";

    return 0;
}