#include "concurrent_task_engine.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <future>
#include <latch>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace {

void test_submit_is_generic_and_propagates_exceptions() {
    ConcurrentTaskEngine engine(3);

    auto number = engine.submit([](int left, int right) { return left + right; }, 19, 23);
    auto text = engine.submit([] { return std::string{"ready"}; });
    auto move_only = engine.submit([value = std::make_unique<int>(7)] { return *value; });
    auto failing = engine.submit([]() -> int { throw std::runtime_error("expected"); });

    assert(number.get() == 42);
    assert(text.get() == "ready");
    assert(move_only.get() == 7);

    try {
        static_cast<void>(failing.get());
        assert(false);
    } catch (const std::runtime_error& error) {
        assert(std::string(error.what()) == "expected");
    }
}

void test_engine_accepts_concurrent_work_and_gracefully_drains() {
    ConcurrentTaskEngine engine(1);
    std::atomic<int> completed{0};
    std::vector<std::thread> submitters;
    for (int producer = 0; producer < 4; ++producer) {
        submitters.emplace_back([&] {
            std::vector<std::future<void>> futures;
            for (int index = 0; index < 100; ++index) {
                futures.push_back(engine.submit([&completed] { completed.fetch_add(1); }));
            }
            for (auto& future : futures) future.get();
        });
    }
    for (auto& submitter : submitters) submitter.join();
    assert(completed.load() == 400);

    std::latch running{1};
    std::latch release{1};
    auto first = engine.submit([&] {
        running.count_down();
        release.wait();
        return 1;
    });
    running.wait();
    auto queued = engine.submit([] { return 2; });

    std::atomic<bool> shutdown_finished{false};
    std::thread shutdown_thread([&] {
        engine.shutdown();
        shutdown_finished.store(true);
    });
    std::this_thread::sleep_for(20ms);
    assert(!shutdown_finished.load());
    release.count_down();
    shutdown_thread.join();

    assert(first.get() == 1);
    assert(queued.get() == 2);
    assert(shutdown_finished.load());

    try {
        static_cast<void>(engine.submit([] { return 3; }));
        assert(false);
    } catch (const std::runtime_error&) {
    }
}

} // namespace

int main() {
    test_submit_is_generic_and_propagates_exceptions();
    test_engine_accepts_concurrent_work_and_gracefully_drains();
}
