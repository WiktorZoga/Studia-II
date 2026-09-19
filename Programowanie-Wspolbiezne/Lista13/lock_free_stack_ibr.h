#pragma once

#include <atomic>
#include <cstddef>
#include <optional>

#include "IBR.h"

template<typename T>
class lock_free_stack_ibr {
private:
    struct node {
        T data;
        node* next;
        std::uint64_t birth_epoch;

        node(const T& data, node* next, std::uint64_t birth_epoch)
            : data(data), next(next), birth_epoch(birth_epoch) {}
    };

    std::atomic<node*> head_{nullptr};
    IBRDomain ibr_;

public:
    explicit lock_free_stack_ibr(std::size_t max_threads) : ibr_(max_threads) {}

    lock_free_stack_ibr(const lock_free_stack_ibr&) = delete;
    lock_free_stack_ibr& operator=(const lock_free_stack_ibr&) = delete;

    ~lock_free_stack_ibr() {
        node* current = head_.exchange(nullptr, std::memory_order_acq_rel);
        while (current) {
            node* next = current->next;
            delete current;
            current = next;
        }
    }

    void push(const T& data) {
        node* new_node = new node{
            data,
            head_.load(std::memory_order_relaxed),
            ibr_.current_epoch(),
        };

        while (!head_.compare_exchange_weak(
            new_node->next,
            new_node,
            std::memory_order_release,
            std::memory_order_relaxed)) {
        }
    }

    std::optional<T> pop() {
        auto guard = ibr_.read_guard();

        while (true) {
            guard.update();
            node* current = head_.load(std::memory_order_acquire);
            if (!current) {
                return std::nullopt;
            }

            node* next = current->next;
            if (!head_.compare_exchange_weak(
                    current,
                    next,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire)) {
                continue;
            }

            try {
                std::optional<T> result{current->data};
                ibr_.retire(current, current->birth_epoch);
                return result;
            } catch (...) {
                ibr_.retire(current, current->birth_epoch);
                throw;
            }
        }
    }
};
