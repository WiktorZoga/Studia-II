#pragma once

#include <cstddef>
#include <mutex>
#include <semaphore>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T>
class ThreadSafeTwoStackQueue {
public:
    static_assert(std::is_nothrow_move_constructible_v<T>,
                  "T must be nothrow move constructible");
    explicit ThreadSafeTwoStackQueue(std::size_t capacity)
        : spaces_(checked_capacity(capacity)), items_(0) {}

    void push(T value) {
        spaces_.acquire();
        push_reserved(std::move(value));
    }

    void try_push(T value) {
        if (!spaces_.try_acquire()) throw std::overflow_error("queue is full");
        push_reserved(std::move(value));
    }

    T pop() {
        items_.acquire();
        return pop_reserved();
    }

    T try_pop() {
        if (!items_.try_acquire()) throw std::underflow_error("queue is empty");
        return pop_reserved();
    }

private:
    static std::ptrdiff_t checked_capacity(std::size_t capacity) {
        if (capacity == 0 || capacity > static_cast<std::size_t>(std::counting_semaphore<>::max())) {
            throw std::invalid_argument("capacity must be positive and within the semaphore limit");
        }
        return static_cast<std::ptrdiff_t>(capacity);
    }
    void push_reserved(T value) {
        try {
            std::lock_guard lock(enqueue_mutex_);
            enqueue_stack_.push_back(std::move(value));
            items_.release();
        } catch (...) {
            spaces_.release();
            throw;
        }
    }

    void transfer_if_needed() {
        // The caller holds dequeue_mutex_; producers only hold enqueue_mutex_.
        if (!dequeue_stack_.empty()) return;
        std::lock_guard lock(enqueue_mutex_);
        dequeue_stack_.reserve(enqueue_stack_.size());
        // Capacity is reserved first; all following moves are non-throwing.
        for (auto it = enqueue_stack_.rbegin(); it != enqueue_stack_.rend(); ++it) {
            dequeue_stack_.push_back(std::move(*it));
        }
        enqueue_stack_.clear();
    }

    T pop_reserved() {
        try {
            std::lock_guard lock(dequeue_mutex_);
            transfer_if_needed();
            T value = std::move(dequeue_stack_.back());
            dequeue_stack_.pop_back();
            spaces_.release();
            return value;
        } catch (...) {
            items_.release();
            throw;
        }
    }
    std::counting_semaphore<> spaces_, items_;
    std::mutex enqueue_mutex_, dequeue_mutex_;
    std::vector<T> enqueue_stack_, dequeue_stack_;
};
