#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <random>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

class WorkStealingTaskEngine {
public:
    explicit WorkStealingTaskEngine(std::size_t thread_count) : queues_(thread_count) {
        if (thread_count == 0) throw std::invalid_argument("thread_count must be positive");
        workers_.reserve(thread_count);
        try { for (std::size_t i = 0; i < thread_count; ++i) workers_.emplace_back([this, i] { worker_loop(i); }); }
        catch (...) { shutdown(); throw; }
    }
    WorkStealingTaskEngine(const WorkStealingTaskEngine&) = delete;
    ~WorkStealingTaskEngine() { shutdown(); }

    template <typename Func, typename... Args>
    auto submit(Func&& function, Args&&... arguments)
        -> std::future<std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>> {
        using result_type = std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>;
        auto invocation = [callable = std::forward<Func>(function), values = std::make_tuple(std::forward<Args>(arguments)...)]() mutable -> result_type {
            return std::apply([&callable](auto&&... args) -> result_type { return std::invoke(std::move(callable), std::move(args)...); }, values);
        };
        std::packaged_task<result_type()> result_task(std::move(invocation));
        auto result = result_task.get_future();
        task_type task([wrapped = std::move(result_task)]() mutable { wrapped(); });
        {
            std::lock_guard life_lock(lifecycle_mutex_);
            if (!accepting_.load()) throw std::runtime_error("engine has been shut down");
            const std::size_t queue_index = next_queue_.fetch_add(1) % queues_.size();
            std::lock_guard queue_lock(queues_[queue_index].mutex);
            queues_[queue_index].tasks.push_back(std::move(task));
            std::lock_guard wait_lock(wait_mutex_);
            queued_tasks_.fetch_add(1);
        }
        work_ready_.notify_one();
        return result;
    }

    void shutdown() {
        std::call_once(shutdown_once_, [this] {
            {
                std::lock_guard life_lock(lifecycle_mutex_);
                std::lock_guard wait_lock(wait_mutex_);
                accepting_.store(false);
            }
            work_ready_.notify_all();
            for (auto& worker : workers_) if (worker.joinable()) worker.join();
        });
    }

private:
    using task_type = std::packaged_task<void()>;
    struct Queue { std::mutex mutex; std::deque<task_type> tasks; };

    bool try_take(std::size_t index, bool owner, task_type& target) {
        std::lock_guard lock(queues_[index].mutex);
        if (queues_[index].tasks.empty()) return false;
        if (owner) { target = std::move(queues_[index].tasks.back()); queues_[index].tasks.pop_back(); }
        else { target = std::move(queues_[index].tasks.front()); queues_[index].tasks.pop_front(); }
        queued_tasks_.fetch_sub(1);
        return true;
    }
    void worker_loop(std::size_t owner) {
        std::minstd_rand random(static_cast<unsigned>(owner + 1));
        while (true) {
            task_type task;
            if (!try_take(owner, true, task)) {
                const std::size_t start = random() % queues_.size();
                for (std::size_t offset = 0; offset < queues_.size() && !task.valid(); ++offset) {
                    const std::size_t victim = (start + offset) % queues_.size();
                    if (victim != owner) try_take(victim, false, task);
                }
            }
            if (task.valid()) { task(); continue; }
            std::unique_lock lock(wait_mutex_);
            work_ready_.wait(lock, [this] { return queued_tasks_.load() != 0 || !accepting_.load(); });
            if (!accepting_.load() && queued_tasks_.load() == 0) return;
        }
    }
    std::vector<Queue> queues_;
    std::vector<std::thread> workers_;
    std::mutex lifecycle_mutex_, wait_mutex_;
    std::condition_variable work_ready_;
    std::atomic<bool> accepting_{true};
    std::atomic<std::size_t> next_queue_{0}, queued_tasks_{0};
    std::once_flag shutdown_once_;
};
