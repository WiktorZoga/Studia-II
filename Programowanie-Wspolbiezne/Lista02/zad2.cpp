#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <vector>

class counting_semaphore {
private:
    int counter;
    std::mutex mtx;
    std::condition_variable has_tokens;

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

    bool try_acquire() noexcept {
        std::lock_guard<std::mutex> lock(mtx);
        if (counter > 0) {
            counter--;
            return true;
        }
        return false;
    }
};

counting_semaphore sem(3);
std::mutex cout_mtx;      
int threads_inside = 0;   

void worker(int id) {
    {
        std::lock_guard<std::mutex> lock(cout_mtx);
        std::cout << "Worker " << id << " czeka na wejście...\n";
    }
    
    sem.acquire(); 
    
    {
        std::lock_guard<std::mutex> lock(cout_mtx);
        threads_inside++;
        std::cout << "-> Worker " << id << " Wszedł. (W środku jest " << threads_inside << "/3)\n";
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    {
        std::lock_guard<std::mutex> lock(cout_mtx);
        threads_inside--;
        std::cout << "<- Worker " << id << " Wychodzi.\n";
    }
    
    sem.release();
}

int main() {
    std::cout << "--- TEST 1: try_acquire ---\n";
    counting_semaphore test_sem(1); 
    
    if (test_sem.try_acquire()) {
        std::cout << "1. try_acquire zwrocilo TRUE [Było miejsce]\n";
    }
    
    if (!test_sem.try_acquire()) {
        std::cout << "2. try_acquire zwrocilo FALSE [Nie było miejca]\n";
    }
    test_sem.release(); 
    
    std::cout << "\n--- TEST 2:  ---\n";
    std::vector<std::thread> workers;
    int const NUMBER_OF_WORKERS = 10;

    for (int i = 0; i < NUMBER_OF_WORKERS; i++) {
        workers.push_back(std::thread(worker, i));
    }

    for (int i = 0; i < NUMBER_OF_WORKERS; i++) {
        if (workers[i].joinable()) {
            workers[i].join();
        }
    }

    std::cout << "\nWszystkie testy zakonczone\n";
    return 0;
}