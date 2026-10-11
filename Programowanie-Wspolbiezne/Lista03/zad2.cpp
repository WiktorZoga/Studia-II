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
    // Queue of type-erased tasks that return int
    std::deque<std::packaged_task<int()>> task_queue;
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
        // STEP 1:
        // 1. Create a std::packaged_task wrapping 'f'
        // 2. Retrieve the associated future
        // 3. Safely add the task to 'task_queue' using std::move
        // 4. Notify the condition variable
        // 5. Return the future
        // --- YOUR CODE HERE ---
        std::packaged_task<int()> task(f);
        std::future<int> result = task.get_future();

        {
            std::lock_guard<std::mutex> lock(mtx);
            task_queue.emplace_back(std::move(task));
        }

        cv.notify_one();

        return result;
    }

    void executor_loop() {
        while (true) {
            std::packaged_task<int()> task;
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this] {return stop || !task_queue.empty(); });
                if (stop && task_queue.empty()) return;
                // STEP 1: Retrieve a task from the queue (remember move semantics!)
                // --- YOUR CODE HERE ---
                task = std::move(task_queue.front());
                task_queue.pop_front();
            }
            // STEP 1: Execute the retrieved task.
            // STEP 2: Do we need try-catch here to protect the executor loop
            // from exceptions thrown by tasks? Implement the appropriate approach
            // and explain it in a comment.
            // --- YOUR CODE HERE ---
            task();

            /* We do not need try-catch.
               std::packaged_task has built-in exception handling.
               If the task throws an exception, task() catches it and stores
               it in the shared state. The exception is rethrown only
               in the thread that calls .get() on the associated
               std::future object.
            */

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
    std::packaged_task<int()> health_check([]{
        SafePrint() << "[Diagnostics] Pinging DB...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return 200; // 200 OK
    });

    for (int i = 0; i < 3; i++) {
        // 1. Retrieve the future (get_future can be called only once for the current state)
        // 2. Call health_check directly
        // 3. Print the result using future.get()
        // 4. Reset health_check for reuse in the next loop iteration
        // --- YOUR CODE HERE ---
        std::future<int> result = health_check.get_future();
        health_check();
        std::cout << result.get() << "\n";
        health_check.reset();
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
