#pragma once

#include <deque>
#include <functional>
#include <mutex>
#include <set>
#include <utility>

template<typename T, typename Compare = std::less<T>>
class HistorySet {
public:
    // Returns whether the set changed. Both successful and unsuccessful
    // insert/erase calls are recorded; contains is only a query.
    bool insert(const T& value) {
        std::scoped_lock lock(mutex_);

        const auto [position, inserted] = values_.insert(value);
        try {
            history_.emplace_back(OperationKind::insert, value, inserted);
        } catch (...) {
            if (inserted) {
                values_.erase(position);
            }
            throw;
        }
        return inserted;
    }

    bool erase(const T& value) {
        std::scoped_lock lock(mutex_);

        const auto position = values_.find(value);
        if (position == values_.end()) {
            history_.emplace_back(OperationKind::erase, value, false);
            return false;
        }

        // Store the actual element before changing the set. Copying T or
        // allocating history storage may throw, but then the set is unchanged.
        history_.emplace_back(OperationKind::erase, *position, true);
        values_.erase(position);
        return true;
    }

    bool contains(const T& value) const {
        std::scoped_lock lock(mutex_);
        return values_.contains(value);
    }

    // Returns false only if there is no insert/erase operation left to undo.
    bool undo() {
        std::scoped_lock lock(mutex_);
        if (history_.empty()) {
            return false;
        }

        const Operation& operation = history_.back();
        if (operation.changed) {
            if (operation.kind == OperationKind::insert) {
                const auto position = values_.find(operation.value);
                if (position != values_.end()) {
                    values_.erase(position);
                }
            } else {
                values_.insert(operation.value);
            }
        }

        history_.pop_back();
        return true;
    }

private:
    enum class OperationKind {
        insert,
        erase,
    };

    struct Operation {
        OperationKind kind;
        T value;
        bool changed;
    };

    mutable std::mutex mutex_;
    std::set<T, Compare> values_;
    std::deque<Operation> history_;
};
