#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <utility>

template<typename T, typename Compare = std::less<T>>
class VersionedConcurrentSet {
private:
    struct Update {
        bool present;
        std::uint64_t version;
    };

    struct State {
        explicit State(Compare compare)
            : compare_(std::move(compare)), values_(compare_) {}

        Compare compare_;
        std::mutex mutex_;
        // An absent entry is a tombstone. Keeping it is what lets a newer
        // erase defeat an older insert that happens to merge later.
        std::map<T, Update, Compare> values_;
        std::atomic<std::uint64_t> next_version_{0};
    };

public:
    class Snapshot {
    public:
        Snapshot(const Snapshot&) = delete;
        Snapshot& operator=(const Snapshot&) = delete;
        Snapshot(Snapshot&&) noexcept = default;
        Snapshot& operator=(Snapshot&&) noexcept = default;

        // A snapshot belongs to one thread. Its normal operations never lock
        // the shared State; only merge() synchronizes with other snapshots.
        bool insert(const T& value) {
            const bool inserted = local_values_.insert(value).second;
            record_update(value, true);
            return inserted;
        }

        // Erasing an absent local value still records a deletion. It is an
        // explicit later write and must therefore win during merge().
        bool erase(const T& value) {
            const bool erased = local_values_.erase(value) != 0;
            record_update(value, false);
            return erased;
        }

        bool find(const T& value) const {
            return local_values_.contains(value);
        }

        void merge() {
            std::scoped_lock lock(state_->mutex_);

            for (const auto& [value, update] : pending_updates_) {
                const auto current = state_->values_.find(value);
                if (current == state_->values_.end() ||
                    current->second.version < update.version) {
                    state_->values_.insert_or_assign(value, update);
                }
            }

            pending_updates_.clear();
        }

    private:
        friend class VersionedConcurrentSet;

        Snapshot(std::shared_ptr<State> state, std::set<T, Compare> local_values)
            : state_(std::move(state)),
              local_values_(std::move(local_values)),
              pending_updates_(state_->compare_) {}

        void record_update(const T& value, bool present) {
            const std::uint64_t version =
                state_->next_version_.fetch_add(1, std::memory_order_relaxed) + 1;
            pending_updates_.insert_or_assign(value, Update{present, version});
        }

        std::shared_ptr<State> state_;
        std::set<T, Compare> local_values_;
        std::map<T, Update, Compare> pending_updates_;
    };

    explicit VersionedConcurrentSet(Compare compare = Compare{})
        : state_(std::make_shared<State>(std::move(compare))) {}

    [[nodiscard]] Snapshot snapshot() const {
        std::set<T, Compare> local_values(state_->compare_);

        {
            std::scoped_lock lock(state_->mutex_);
            for (const auto& [value, update] : state_->values_) {
                if (update.present) {
                    local_values.insert(value);
                }
            }
        }

        return Snapshot(state_, std::move(local_values));
    }

private:
    std::shared_ptr<State> state_;
};
