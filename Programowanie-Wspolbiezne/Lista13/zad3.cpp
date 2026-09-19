#include <atomic>
#include <barrier>
#include <cassert>
#include <stdexcept>
#include <thread>
#include <vector>

#include "history_set.h"

struct CopyMayThrow {
    int value;
    inline static int copies_before_throw = -1;

    CopyMayThrow(int value) : value(value) {}

    CopyMayThrow(const CopyMayThrow& other) : value(other.value) {
        if (copies_before_throw == 0) {
            throw std::runtime_error("copy failed");
        }
        if (copies_before_throw > 0) {
            --copies_before_throw;
        }
    }

    friend bool operator<(const CopyMayThrow& left, const CopyMayThrow& right) {
        return left.value < right.value;
    }
};

int main() {
    {
        HistorySet<int> set;

        assert(set.insert(10));
        assert(!set.insert(10));
        assert(set.erase(10));
        assert(!set.erase(10));
        assert(!set.contains(10));

        assert(set.undo());
        assert(!set.contains(10));
        assert(set.undo());
        assert(set.contains(10));
        assert(set.undo());
        assert(set.contains(10));
        assert(set.undo());
        assert(!set.contains(10));
        assert(!set.undo());
    }

    {
        HistorySet<CopyMayThrow> set;
        const CopyMayThrow value{5};

        CopyMayThrow::copies_before_throw = 1;
        bool threw = false;
        try {
            set.insert(value);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        CopyMayThrow::copies_before_throw = -1;

        assert(threw);
        assert(!set.contains(value));
        assert(set.insert(value));
        assert(set.contains(value));
    }

    {
        constexpr int worker_count = 8;
        constexpr int values_per_worker = 500;
        HistorySet<int> set;
        std::barrier start(worker_count + 1);

        {
            std::vector<std::jthread> workers;
            workers.reserve(worker_count);
            for (int worker = 0; worker < worker_count; ++worker) {
                workers.emplace_back([&, worker] {
                    start.arrive_and_wait();
                    for (int value = 0; value < values_per_worker; ++value) {
                        assert(set.insert(worker * values_per_worker + value));
                    }
                });
            }
            start.arrive_and_wait();
        }

        for (int value = 0; value < worker_count * values_per_worker; ++value) {
            assert(set.contains(value));
        }
    }
}
