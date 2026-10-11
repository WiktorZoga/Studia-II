#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

class ConcurrentTaskEngine {
public:
    explicit ConcurrentTaskEngine(std::size_t thread_count) {
        if (thread_count == 0) {
            throw std::invalid_argument("thread_count must be positive");
        }

        workers_.reserve(thread_count);
        try {
            for (std::size_t index = 0; index < thread_count; ++index) {
                workers_.emplace_back([this] { worker_loop(); });
            }
        } catch (...) {
            shutdown();
            throw;
        }
    }

    ConcurrentTaskEngine(const ConcurrentTaskEngine&) = delete;
    ConcurrentTaskEngine& operator=(const ConcurrentTaskEngine&) = delete;

    ~ConcurrentTaskEngine() {
        shutdown();
    }

    template <typename Func, typename... Args>
    auto submit(Func&& function, Args&&... arguments)
        -> std::future<std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>> {
        using result_type = std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>;

        auto invocation = [callable = std::forward<Func>(function),
                           stored_arguments = std::make_tuple(std::forward<Args>(arguments)...)]() mutable -> result_type {
            return std::apply(
                [&callable](auto&&... values) -> result_type {
                    return std::invoke(std::move(callable), std::move(values)...);
                },
                stored_arguments);
        };

        std::packaged_task<result_type()> result_task(std::move(invocation));
        auto result = result_task.get_future();
        task_type task([wrapped = std::move(result_task)]() mutable { wrapped(); });

        {
            std::lock_guard lock(mutex_);
            if (!accepting_work_) {
                throw std::runtime_error("engine has been shut down");
            }
            tasks_.emplace_back(std::move(task));
        }
        condition_.notify_one();
        return result;
    }

    void shutdown() {
        std::call_once(shutdown_once_, [this] {
            {
                std::lock_guard lock(mutex_);
                accepting_work_ = false;
            }
            condition_.notify_all();
            for (auto& worker : workers_) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
        });
    }

private:
    using task_type = std::packaged_task<void()>;

    void worker_loop() {
        while (true) {
            task_type task;
            {
                std::unique_lock lock(mutex_);
                condition_.wait(lock, [this] {
                    return !tasks_.empty() || !accepting_work_;
                });
                if (tasks_.empty()) {
                    return;
                }
                task = std::move(tasks_.front());
                tasks_.pop_front();
            }
            task();
        }
    }

    std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<task_type> tasks_;
    std::vector<std::thread> workers_;
    bool accepting_work_ = true;
    std::once_flag shutdown_once_;
};
