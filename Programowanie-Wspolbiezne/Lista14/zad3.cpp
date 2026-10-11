#include "concurrent_task_engine.h"
#include "parallel_merge_sort.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <random>
#include <vector>
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>

namespace {
class RejectSecondSubmission {
public:
    std::atomic<bool> completed{false};
    template <typename Func>
    std::future<void> submit(Func&& function) {
        if (++submissions_ == 2) throw std::runtime_error("submission rejected");
        return engine_.submit([this, task = std::forward<Func>(function)]() mutable {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            task();
            completed.store(true);
        });
    }
private:
    int submissions_ = 0;
    ConcurrentTaskEngine engine_{1};
};

void test_submission_failure_drains_existing_jobs() {
    RejectSecondSubmission engine;
    std::vector<int> values{4, 3, 2, 1};
    bool rejected = false;
    try { parallel_merge_sort(values.begin(), values.end(), engine); }
    catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
    assert(engine.completed.load());
}

struct Item { int key; int id; };
bool operator<(const Item& left, const Item& right) { return left.key < right.key; }

void test_parallel_merge_sort() {
    ConcurrentTaskEngine engine(4);
    std::vector<bool> bits{true, false, true, false, false};
    parallel_merge_sort(bits.begin(), bits.end(), engine);
    assert(std::is_sorted(bits.begin(), bits.end()));
    bool flags[]{true, false, true, false};
    parallel_merge_sort(std::begin(flags), std::end(flags), engine);
    assert(std::is_sorted(std::begin(flags), std::end(flags)));
    std::vector<int> empty;
    parallel_merge_sort(empty.begin(), empty.end(), engine);
    std::vector<int> values{9, 1, 7, 1, 3, 2, 8, 0, 5, 4, 6};
    parallel_merge_sort(values.begin(), values.end(), engine);
    assert(std::is_sorted(values.begin(), values.end()));
    std::vector<Item> items{{2, 1}, {1, 2}, {2, 3}, {0, 4}};
    parallel_merge_sort(items.begin(), items.end(), engine);
    assert(std::is_sorted(items.begin(), items.end()));
    std::mt19937 generator(7);
    std::vector<int> random(5000);
    for (int& value : random) value = static_cast<int>(generator());
    auto expected = random;
    std::sort(expected.begin(), expected.end());
    parallel_merge_sort(random.begin(), random.end(), engine);
    assert(random == expected);
    for (std::vector<int> edge : {std::vector<int>{42}, std::vector<int>{5, 4, 3, 2, 1}, std::vector<int>(100, 7)}) {
        auto expected_edge = edge;
        std::sort(expected_edge.begin(), expected_edge.end());
        parallel_merge_sort(edge.begin(), edge.end(), engine);
        assert(edge == expected_edge);
    }
}
} // namespace
int main() {
    test_parallel_merge_sort();
    test_submission_failure_drains_existing_jobs();
}
