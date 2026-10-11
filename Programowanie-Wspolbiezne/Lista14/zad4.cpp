#include "work_stealing_task_engine.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <future>
#include <latch>
#include <stdexcept>
#include <vector>
#include <thread>

void test_concurrent_submission_and_draining_shutdown() {
    WorkStealingTaskEngine engine(1);
    std::vector<std::thread> submitters;
    std::atomic<int> completed{0};
    for (int producer = 0; producer < 4; ++producer) {
        submitters.emplace_back([&] {
            std::vector<std::future<void>> results;
            for (int i = 0; i < 100; ++i) {
                results.push_back(engine.submit([&] { completed.fetch_add(1); }));
            }
            for (auto& result : results) result.get();
        });
    }
    for (auto& submitter : submitters) submitter.join();
    assert(completed.load() == 400);

    std::latch running{1}, release{1};
    auto blocker = engine.submit([&] { running.count_down(); release.wait(); });
    running.wait();
    auto queued = engine.submit([] { return 42; });
    auto stopping = std::async(std::launch::async, [&] { engine.shutdown(); });
    assert(stopping.wait_for(std::chrono::milliseconds(20)) == std::future_status::timeout);
    release.count_down();
    stopping.get();
    blocker.get();
    assert(queued.get() == 42);
    engine.shutdown();
}

int main() {
    test_concurrent_submission_and_draining_shutdown();
    WorkStealingTaskEngine engine(3);
    std::atomic<int> completed{0};
    std::vector<std::future<int>> futures;
    for (int index = 0; index < 90; ++index) {
        futures.push_back(engine.submit([index, &completed] {
            completed.fetch_add(1);
            return index * 2;
        }));
    }
    for (int index = 0; index < 90; ++index) assert(futures[index].get() == index * 2);
    assert(completed.load() == 90);
    auto failing = engine.submit([]() -> int { throw std::runtime_error("expected"); });
    try { static_cast<void>(failing.get()); assert(false); } catch (const std::runtime_error&) {}
    engine.shutdown();
    try { static_cast<void>(engine.submit([] {})); assert(false); } catch (const std::runtime_error&) {}

    WorkStealingTaskEngine stealing_engine(2);
    std::latch blocker_started{1};
    std::latch release_blocker{1};
    auto blocker = stealing_engine.submit([&] { blocker_started.count_down(); release_blocker.wait(); });
    blocker_started.wait();
    auto other_worker_task = stealing_engine.submit([] { return 1; });
    auto queued_for_blocked_worker = stealing_engine.submit([] { return 2; });
    assert(other_worker_task.get() == 1);
    assert(queued_for_blocked_worker.wait_for(std::chrono::milliseconds(100)) == std::future_status::ready);
    assert(queued_for_blocked_worker.get() == 2);
    release_blocker.count_down();
    blocker.get();
}
