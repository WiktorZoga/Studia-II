#include <iostream>
#include <future>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <chrono>
#include <stdexcept>
#include <sstream>

class SafePrint {
public:
    SafePrint() = default;

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

class TaskExecutor {
    // A promise is an empty, non-copyable container; it cannot wrap a function,
    // so the queue holds functions (tasks) that refer to their corresponding promises
    std::deque<std::function<void()>> task_queue;
    std::mutex mtx;
    std::condition_variable cv;
    bool stop = false;

    // Multithreading infrastructure
    std::vector<std::thread> workers;
public:
    // Constructor that starts the executor's thread pool
    explicit TaskExecutor(size_t num_threads = 2) {
        for (size_t i = 0; i < num_threads; i++) {
            workers.emplace_back(&TaskExecutor::executor_loop, this);
        }
    }

    // Destructor that ensures threads are safely joined
    ~TaskExecutor() {
        shutdown();
    }

    template<typename Func>
    std::future<int> submit_task(Func f) {
        auto promise = std::make_shared<std::promise<int>>();

        std::future<int> result = promise->get_future();

        // This is effectively a manually implemented packaged_task
        auto task_wrapper = [promise_ = std::move(promise), f_ = std::move(f)]() {
            try {
                promise_->set_value(f_());
            } catch (...) {
                promise_->set_exception(std::current_exception());
            }
        };

        {
            std::lock_guard<std::mutex> lock(mtx);
            task_queue.emplace_back(std::move(task_wrapper));
        }

        cv.notify_one();

        return result;
    }

    void executor_loop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this] {return stop || !task_queue.empty(); });
                if (stop && task_queue.empty()) return;
                task = std::move(task_queue.front());
                task_queue.pop_front();
            }
            task();
        }
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (stop) return; // Prevent repeated calls from the destructor
            stop = true;
        }
        cv.notify_all();
        for (auto& t: workers) {
            if (t.joinable()) t.join();
        }
    }
};

void diagnostic_worker() {
    // STEP 3:
    // std::packaged_task<int()> health_check([]{
    //     SafePrint() << "[Diagnostics] Pinging DB...\n";
    //     std::this_thread::sleep_for(std::chrono::milliseconds(50));
    //     return 200; // 200 OK
    // });

    auto health_check = [](std::promise<int>& promise) {
        SafePrint() << "[Diagnostics] Pinging DB...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        promise.set_value(200);
    };

    for (int i = 0; i < 3; i++) {

        std::promise<int> promise;
        std::future<int> result = promise.get_future();

        health_check(promise);

        SafePrint() << result.get() << "\n";
    }
}

int main() {
    std::cout << "--- System Start ---\n";
    TaskExecutor executor(2); // Initialize the executor with two worker threads
    std::cout << "\nSubmitting tasks to the executor...\n";
    // Submit a successful task
    auto fut1 = executor.submit_task([] {
        SafePrint() << "[Task 1] Committing transaction A...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 1;
    });
    // Submit a task that throws an exception (verify STEP 2)
    auto fut2 = executor.submit_task([] {
        SafePrint() << "[Task 2] Committing transaction B...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        throw std::runtime_error("DB Lockout");
        return 2;
    });
    // Retrieve results and verify exception propagation
    try {
        if (fut1.valid())   SafePrint() << "Task 1 result: " << fut1.get() << "\n";
    } catch (const std::exception& e) {
        SafePrint() << "Task 1 reported an error: " << e.what() << "\n";
    }

    try {
        if (fut2.valid())   SafePrint() << "Task 2 result: " << fut2.get() << "\n";
    } catch (const std::exception& e) {
        SafePrint() << "Task 2 threw an exception (EXPECTED behavior in Step 2): " << e.what() << "\n";
    }

    // Test STEP 3 (Diagnostics using reset)
    SafePrint() << "\n--- Running the diagnostic test ---\n";
    std::thread diag_thread(diagnostic_worker);
    diag_thread.join();

    // Manually shut down the executor (optional; the destructor also does this)
    executor.shutdown();
    std::cout << "\n--- System End ---\n";
    return 0;
}
