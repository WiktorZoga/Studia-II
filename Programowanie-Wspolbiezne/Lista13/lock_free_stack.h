#pragma once

#include <atomic>
#include <optional>

#include "EBR.h"

template<typename T>
class lock_free_stack {
private:
    struct node {
        T data;
        node* next;

        node(const T& data, node* next = nullptr) : data(data), next(next) {}
    };

    std::atomic<node*> head{nullptr};
    EBRDomain ebr;

public:
    explicit lock_free_stack(std::size_t max_threads) : ebr(max_threads) {}

    lock_free_stack(const lock_free_stack&) = delete;
    lock_free_stack& operator=(const lock_free_stack&) = delete;

    ~lock_free_stack() {
        node* current = head.exchange(nullptr);

        while (current) {
            node* next = current->next;
            delete current;
            current = next;
        }
    }

    void push(const T& data) {
        node* new_node = new node{data, head.load(std::memory_order_relaxed)};

        while (!head.compare_exchange_weak(
            new_node->next, new_node,
            std::memory_order_release, std::memory_order_relaxed)) {
        }
    }

    std::optional<T> pop() {
        auto guard = ebr.read_guard();
        node* current_head = head.load(std::memory_order_acquire);

        while (current_head && !head.compare_exchange_weak(
            current_head, current_head->next,
            std::memory_order_acq_rel, std::memory_order_acquire)) {
        }

        if (!current_head) {
            return std::nullopt;
        }

        try {
            std::optional<T> result{current_head->data};
            ebr.retire(current_head);
            return result;
        } catch (...) {
            ebr.retire(current_head);
            throw;
        }
    }
};
