#pragma once

#include <condition_variable>
#include <mutex>

class ResourceManager {
public:
    void start_search() {
        std::unique_lock lock(mutex_);
        condition_.wait(lock, [this] {
            return !delete_active_ && waiting_deleters_ == 0;
        });
        ++active_searchers_;
    }

    void end_search() {
        std::lock_guard lock(mutex_);
        --active_searchers_;
        condition_.notify_all();
    }

    void start_insert() {
        std::unique_lock lock(mutex_);
        condition_.wait(lock, [this] {
            return !delete_active_ && waiting_deleters_ == 0 && !insert_active_;
        });
        insert_active_ = true;
    }

    void end_insert() {
        std::lock_guard lock(mutex_);
        insert_active_ = false;
        condition_.notify_all();
    }

    void start_delete() {
        std::unique_lock lock(mutex_);
        ++waiting_deleters_;
        condition_.wait(lock, [this] {
            return !delete_active_ && !insert_active_ && active_searchers_ == 0;
        });
        --waiting_deleters_;
        delete_active_ = true;
    }

    void end_delete() {
        std::lock_guard lock(mutex_);
        delete_active_ = false;
        condition_.notify_all();
    }

private:
    std::mutex mutex_;
    std::condition_variable condition_;
    unsigned active_searchers_ = 0;
    unsigned waiting_deleters_ = 0;
    bool insert_active_ = false;
    bool delete_active_ = false;
};
