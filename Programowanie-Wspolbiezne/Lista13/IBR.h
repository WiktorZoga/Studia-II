#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <vector>

constexpr std::uint64_t IBR_INACTIVE = static_cast<std::uint64_t>(-1);

struct IBRNode {
    void* pointer;
    std::uint64_t birth_epoch;
    std::uint64_t retire_epoch;
    void (*deleter)(void*);
    IBRNode* next;
};

struct alignas(64) ThreadReservation {
    std::atomic<std::uint64_t> lower_epoch{IBR_INACTIVE};
    std::atomic<std::uint64_t> upper_epoch{IBR_INACTIVE};
};

class IBRDomain {
public:
    explicit IBRDomain(std::size_t max_threads)
        : reservations_(max_threads), domain_id_(next_domain_id_.fetch_add(1)) {}

    IBRDomain(const IBRDomain&) = delete;
    IBRDomain& operator=(const IBRDomain&) = delete;

    class ReadGuard {
    public:
        explicit ReadGuard(IBRDomain& domain) : domain_(domain) {
            domain_.read_lock();
        }

        ReadGuard(const ReadGuard&) = delete;
        ReadGuard& operator=(const ReadGuard&) = delete;

        ~ReadGuard() {
            domain_.read_unlock();
        }

        void update() {
            domain_.read_update();
        }

    private:
        IBRDomain& domain_;
    };

    [[nodiscard]] ReadGuard read_guard() {
        return ReadGuard(*this);
    }

    std::uint64_t current_epoch() const {
        return global_epoch_.load(std::memory_order_acquire);
    }

    template<typename T>
    void retire(T* pointer, std::uint64_t birth_epoch) {
        const std::uint64_t retire_epoch =
            global_epoch_.fetch_add(1, std::memory_order_acq_rel) + 1;

        IBRNode* node = new IBRNode{
            pointer,
            birth_epoch,
            retire_epoch,
            [](void* value) { delete static_cast<T*>(value); },
            nullptr,
        };

        push_retired(node);
        reclaim();
    }

    ~IBRDomain() {
        IBRNode* current = retired_head_.exchange(nullptr, std::memory_order_acq_rel);
        while (current) {
            IBRNode* node = current;
            current = current->next;
            node->deleter(node->pointer);
            delete node;
        }
    }

private:
    std::atomic<std::uint64_t> global_epoch_{0};
    std::vector<ThreadReservation> reservations_;
    std::atomic<IBRNode*> retired_head_{nullptr};
    std::atomic<std::size_t> next_slot_{0};
    const std::uint64_t domain_id_;

    inline static std::atomic<std::uint64_t> next_domain_id_{0};
    inline static thread_local std::unordered_map<std::uint64_t, std::size_t>
        slots_by_domain_;

    void read_lock() {
        const std::size_t slot = current_thread_slot();
        const std::uint64_t epoch = global_epoch_.load(std::memory_order_acquire);
        reservations_[slot].lower_epoch.store(epoch, std::memory_order_release);
        reservations_[slot].upper_epoch.store(epoch, std::memory_order_release);
    }

    void read_update() {
        const std::size_t slot = current_thread_slot();
        const std::uint64_t epoch = global_epoch_.load(std::memory_order_acquire);
        reservations_[slot].upper_epoch.store(epoch, std::memory_order_release);
    }

    void read_unlock() {
        const std::size_t slot = current_thread_slot();
        reservations_[slot].upper_epoch.store(IBR_INACTIVE, std::memory_order_release);
        reservations_[slot].lower_epoch.store(IBR_INACTIVE, std::memory_order_release);
    }

    std::size_t current_thread_slot() {
        const auto found = slots_by_domain_.find(domain_id_);
        if (found != slots_by_domain_.end()) {
            return found->second;
        }

        const std::size_t slot = next_slot_.fetch_add(1, std::memory_order_relaxed);
        if (slot >= reservations_.size()) {
            throw std::length_error("IBR thread-slot capacity exceeded");
        }

        slots_by_domain_.emplace(domain_id_, slot);
        return slot;
    }

    void push_retired(IBRNode* node) {
        IBRNode* expected = retired_head_.load(std::memory_order_relaxed);
        do {
            node->next = expected;
        } while (!retired_head_.compare_exchange_weak(
            expected, node, std::memory_order_release, std::memory_order_relaxed));
    }

    void reclaim() {
        struct Interval {
            std::uint64_t lower;
            std::uint64_t upper;
        };

        std::vector<Interval> active_intervals;
        active_intervals.reserve(reservations_.size());
        for (const auto& reservation : reservations_) {
            const std::uint64_t lower =
                reservation.lower_epoch.load(std::memory_order_acquire);
            if (lower != IBR_INACTIVE) {
                const std::uint64_t upper =
                    reservation.upper_epoch.load(std::memory_order_acquire);
                active_intervals.push_back({lower, upper});
            }
        }

        IBRNode* current = retired_head_.exchange(nullptr, std::memory_order_acquire);
        IBRNode* keep = nullptr;

        while (current) {
            IBRNode* node = current;
            current = current->next;

            bool overlaps_active_reader = false;
            for (const Interval& interval : active_intervals) {
                const bool is_before_reader = node->retire_epoch < interval.lower;
                const bool is_after_reader = node->birth_epoch > interval.upper;
                if (!is_before_reader && !is_after_reader) {
                    overlaps_active_reader = true;
                    break;
                }
            }

            if (overlaps_active_reader) {
                node->next = keep;
                keep = node;
            } else {
                node->deleter(node->pointer);
                delete node;
            }
        }

        while (keep) {
            IBRNode* node = keep;
            keep = keep->next;
            push_retired(node);
        }
    }
};
