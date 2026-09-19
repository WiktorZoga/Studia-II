#include <barrier>
#include <cassert>
#include <thread>
#include <vector>

#include "versioned_concurrent_set.h"

int main() {
    VersionedConcurrentSet<int> set;

    {
        auto snapshot = set.snapshot();
        assert(!snapshot.find(10));
        assert(snapshot.insert(10));
        assert(snapshot.find(10));
        snapshot.merge();
    }

    {
        auto older_snapshot = set.snapshot();
        auto newer_snapshot = set.snapshot();

        older_snapshot.insert(20);
        newer_snapshot.erase(20);

        // The newer erase wins even though it is merged first.
        newer_snapshot.merge();
        older_snapshot.merge();

        assert(!set.snapshot().find(20));
        assert(set.snapshot().find(10));
    }

    {
        constexpr int worker_count = 8;
        std::barrier start(worker_count + 1);

        {
            std::vector<std::jthread> workers;
            workers.reserve(worker_count);
            for (int worker = 0; worker < worker_count; ++worker) {
                workers.emplace_back([&, worker] {
                    auto snapshot = set.snapshot();
                    start.arrive_and_wait();
                    snapshot.insert(100 + worker);
                    snapshot.merge();
                });
            }
            start.arrive_and_wait();
        }

        auto result = set.snapshot();
        for (int worker = 0; worker < worker_count; ++worker) {
            assert(result.find(100 + worker));
        }
    }
}
