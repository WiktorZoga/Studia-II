#include <semaphore>
#include <mutex>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <iostream>
#include <sstream>
#include <iomanip>

template<typename CompletionFunction = std::function<void()>>
class Barrier {
private:
    CompletionFunction func;
    std::mutex mutex;
    std::counting_semaphore<> semaphore_in, semaphore_out;
    int N;
    int counter;
public:
    Barrier(int N_, CompletionFunction func_) : N(N_), func(func_), counter(0), semaphore_in(0), semaphore_out(0) {}

    Barrier(const Barrier&) = delete;
    Barrier& operator=(const Barrier&) = delete;

    Barrier(Barrier&&) = delete;
    Barrier& operator=(Barrier&&) = delete;

    void arrive_and_wait() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            counter++;
            if (counter == N) {
                if (func) func();
                semaphore_in.release(N);
            }
        }
        semaphore_in.acquire();
        {
            std::lock_guard<std::mutex> lock(mutex);
            counter--;
            if (counter == 0) {
                semaphore_out.release(N);
            }
        }
        semaphore_out.acquire();
    }
};

// Globalny punkt startu programu (do mierzenia relatywnego czasu)
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

int main() {
    int const N = 10;
    int const ITERATIONS = 5;

    auto on_completion = []() {
        SafePrint() << "[SYSTEM] Wszystkie wątki dotarły do bariery. Przechodimy przez barierę i zaczynamy kolejna fazę.\n";
    };

    Barrier<> barrier(N, on_completion);

    std::vector<std::jthread> threads;

    for (int i = 1; i <= N; i++) {
        threads.emplace_back([&barrier, i, ITERATIONS](){
            for (int it = 1; it <= ITERATIONS; it++) {
                SafePrint() << "[Wątek " << i << "] " << "START: " << it << "\n";

                // wątki parzyste szybkie, reszta wolna
                std::this_thread::sleep_for(std::chrono::milliseconds(100 + 500 * (i & 1)));

                SafePrint() << "[Wątek " << i << "] " << "END: " << it << "\n";

                barrier.arrive_and_wait();

                SafePrint() << "[Wątek " << i << "] " << "Przeszedł prze barierę:  " << it << "\n";
            }
        });
    }

    for (auto& t: threads) t.join();

    std::cout << "[SYSTEM] Koniec testowania.\n";
}