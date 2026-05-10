#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <vector>
#include <string>

class counting_semaphore {
private:
    int counter;
    std::mutex mtx;
    std::condition_variable_any has_tokens;

public:
    counting_semaphore(int counter_ = 0) : counter(counter_) {}

    counting_semaphore(const counting_semaphore&) = delete;
    counting_semaphore(counting_semaphore&&) = delete;

    counting_semaphore& operator=(const counting_semaphore&) = delete;
    counting_semaphore& operator=(counting_semaphore&&) = delete;

    void release(std::ptrdiff_t update = 1) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            counter += update;
        }
        has_tokens.notify_all();
    }

    void acquire() {
        std::unique_lock<std::mutex> lock(mtx);
        has_tokens.wait(lock, [this](){
            return counter > 0;
        });
        counter--;
    }

    bool acquire_interruptible(std::stop_token stoken) {
        std::unique_lock<std::mutex> lock(mtx);
        bool succes = has_tokens.wait(lock, stoken, [this]() {
            return counter > 0;
        });

        if (!succes) {
            return false;
        }

        counter--;
        return true;
    }

    bool try_acquire() noexcept {
        std::lock_guard<std::mutex> lock(mtx);
        if (counter > 0) {
            counter--;
            return true;
        }
        return false;
    }
};

counting_semaphore sem(0);
std::mutex cout_mtx;      
int threads_inside = 0;   

void worker(std::stop_token stoken, int id) {
    std::cout << "Worker " << id << ": Czeka na żeton\n";

    if (sem.acquire_interruptible(stoken)) {
        std::cout << "Worker " << id << ": Zdobył żeton\n";
        sem.release();
    } else {
        std::cout << "Worker " << id << ": Przerwano mu czekanie\n";
    }
}

int main() {
    std::cout << "[MAIN] Start\n";
    {
        std::jthread t1(worker, 1);
        std::string input;
        while (true) {
            std::cin >> input;
            if (input == "stop") {
                std::cout << "[MAIN] STOP\n";
                t1.request_stop();
                break;
            } else if (input == "go") {
                std::cout << "[MAIN] GO\n";
                sem.release();
                break;
            } else {
                std::cout << "[MAIN] UNDEFINED\n";
            }
        }
    }
    std::cout << "[MAIN] Koniec\n";
    return 0;
}