#pragma once

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <unordered_map>
#include <vector>

constexpr uint64_t EBR_INACTIVE = static_cast<uint64_t>(-1);

struct EBRNode
{
    void* ptr;
    uint64_t retire_epoch;
    void (*deleter) (void*);
    EBRNode* next;
};

class EBRDomain
{
private:
    std::atomic<uint64_t> global_epoch{0};
    std::vector<std::atomic<uint64_t>> thread_epochs;
    std::atomic<EBRNode*> retire_list_head{nullptr};

    std::atomic<std::size_t> next_slot{0};
    const uint64_t domain_id;

    inline static std::atomic<uint64_t> next_domain_id{0};
    inline static thread_local std::unordered_map<uint64_t, std::size_t>
        slots_by_domain;

public:
    explicit EBRDomain(std::size_t max_threads)
        : thread_epochs(max_threads),
          domain_id(next_domain_id.fetch_add(1))
    {
        for (auto& te : thread_epochs) {
            te.store(EBR_INACTIVE);
        }
    }

    void read_lock() {
        const std::size_t slot = current_thread_slot();
        const uint64_t epoch = global_epoch.load(std::memory_order_acquire);
        thread_epochs[slot].store(epoch, std::memory_order_release);
    }

    void read_unlock() {
        const std::size_t slot = current_thread_slot();
        thread_epochs[slot].store(EBR_INACTIVE, std::memory_order_release);
    }

    class ReadGuard {
    public:
        explicit ReadGuard(EBRDomain& domain) : domain_(domain) {
            domain_.read_lock();
        }

        ReadGuard(const ReadGuard&) = delete;
        ReadGuard& operator=(const ReadGuard&) = delete;

        ~ReadGuard() {
            domain_.read_unlock();
        }

    private:
        EBRDomain& domain_;
    };

    [[nodiscard]] ReadGuard read_guard() {
        return ReadGuard(*this);
    }

    template<typename T>
    void retire(T* old_ptr) {
        const uint64_t current_epoch = global_epoch.load(std::memory_order_acquire);

        EBRNode* node = new EBRNode {
            old_ptr, current_epoch, [](void* p) { delete static_cast<T*>(p); }, nullptr
        };

        EBRNode* expected = retire_list_head.load(std::memory_order_relaxed);
        do {
            node->next = expected;
        } while (!retire_list_head.compare_exchange_weak(
            expected, node, std::memory_order_release, std::memory_order_relaxed));

        reclaim();
    }

    ~EBRDomain() {
        EBRNode* current_list = retire_list_head.exchange(nullptr, std::memory_order_acq_rel);

        while (current_list) {
            EBRNode* node = current_list;
            current_list = current_list->next;
            node->deleter(node->ptr);
            delete node;
        }
    }

private:
    std::size_t register_current_thread() {
        std::size_t slot = next_slot.fetch_add(1, std::memory_order_relaxed);
        if (slot >= thread_epochs.size()) {
            throw std::length_error("EBR thread-slot capacity exceeded");
        }
        return slot;
    }

    std::size_t current_thread_slot() {
        auto found = slots_by_domain.find(domain_id);
        if (found != slots_by_domain.end()) {
            return found->second;
        }

        const std::size_t slot = register_current_thread();
        slots_by_domain.emplace(domain_id, slot);
        return slot;
    }

    void reclaim() {
        uint64_t current_epoch = global_epoch.load(std::memory_order_acquire);
        bool can_advance = true;
        uint64_t min_active_epoch = current_epoch;

        for (const auto& te : thread_epochs) {
            const uint64_t t_epoch = te.load(std::memory_order_acquire);
            if (t_epoch != EBR_INACTIVE) {
                if (t_epoch < min_active_epoch) min_active_epoch = t_epoch;
                if (t_epoch != current_epoch) can_advance = false;
            }
        }

        if (can_advance &&
            global_epoch.compare_exchange_strong(
                current_epoch, current_epoch + 1,
                std::memory_order_acq_rel, std::memory_order_acquire)) {
        }

        EBRNode* current_list = retire_list_head.exchange(nullptr, std::memory_order_acquire);
        EBRNode* to_delete = nullptr;
        EBRNode* to_keep = nullptr;

        while (current_list) {
            EBRNode* node = current_list;
            current_list = current_list->next;

            if (node->retire_epoch < min_active_epoch) {
                node->next = to_delete;
                to_delete = node;
            } else {
                node->next = to_keep;
                to_keep = node;
            }
        }

        while (to_delete) {
            EBRNode* next = to_delete->next;
            to_delete->deleter(to_delete->ptr);
            delete to_delete;
            to_delete = next;
        }

        while (to_keep) {
            EBRNode* node = to_keep;
            to_keep = to_keep->next;
            EBRNode* expected = retire_list_head.load(std::memory_order_relaxed);
            do {
                node->next = expected;
            } while (!retire_list_head.compare_exchange_weak(
                expected, node, std::memory_order_release, std::memory_order_relaxed));
        }
    }
};
