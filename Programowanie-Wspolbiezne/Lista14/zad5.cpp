#include "thread_safe_two_stack_queue.h"

#include <atomic>
#include <cassert>
#include <stdexcept>
#include <thread>
#include <future>
#include <chrono>
#include <latch>
#include <vector>
#include <semaphore>

struct CopyMayThrow {
    int value;
    inline static bool fail_copy = false;
    explicit CopyMayThrow(int number) : value(number) {}
    CopyMayThrow(const CopyMayThrow& other) : value(other.value) {
        if (fail_copy) throw std::runtime_error("copy failed");
    }
    CopyMayThrow(CopyMayThrow&& other) noexcept : value(other.value) {}
};

void test_failed_argument_copy_preserves_queue() {
    ThreadSafeTwoStackQueue<CopyMayThrow> queue(1);
    CopyMayThrow input(7);
    CopyMayThrow::fail_copy = true;
    bool failed = false;
    try { queue.push(input); }
    catch (const std::runtime_error&) { failed = true; }
    assert(failed);
    CopyMayThrow::fail_copy = false;
    queue.try_push(input);
    assert(queue.pop().value == 7);
}

void test_capacity_validation() {
    for (std::size_t capacity : {std::size_t{0}, static_cast<std::size_t>(std::counting_semaphore<>::max()) + 1}) {
        bool rejected = false;
        try { ThreadSafeTwoStackQueue<int> invalid(capacity); }
        catch (const std::invalid_argument&) { rejected = true; }
        assert(rejected);
    }
}

void test_blocking_wakeups() {
    using namespace std::chrono_literals;
    ThreadSafeTwoStackQueue<int> queue(1);
    queue.push(10);
    std::latch attempting_push{1};
    auto producer = std::async(std::launch::async, [&] { attempting_push.count_down(); queue.push(20); });
    attempting_push.wait();
    assert(producer.wait_for(20ms) == std::future_status::timeout);
    assert(queue.pop() == 10);
    producer.get();
    assert(queue.pop() == 20);
    std::latch attempting_pop{1};
    auto consumer = std::async(std::launch::async, [&] { attempting_pop.count_down(); return queue.pop(); });
    attempting_pop.wait();
    assert(consumer.wait_for(20ms) == std::future_status::timeout);
    queue.push(30);
    assert(consumer.get() == 30);
}

int main() {
    test_capacity_validation();
    test_blocking_wakeups();
    test_failed_argument_copy_preserves_queue();
    ThreadSafeTwoStackQueue<int> queue(2);
    try { static_cast<void>(queue.try_pop()); assert(false); } catch (const std::underflow_error&) {}
    queue.try_push(1); queue.try_push(2);
    try { queue.try_push(3); assert(false); } catch (const std::overflow_error&) {}
    assert(queue.pop() == 1); assert(queue.pop() == 2);
    ThreadSafeTwoStackQueue<int> concurrent_queue(16);
    constexpr int producer_count = 4;
    constexpr int per_producer = 1000;
    std::vector<std::atomic<int>> seen(producer_count * per_producer);
    for (auto& count : seen) count.store(0);
    std::vector<std::thread> producers, consumers;
    for (int id = 0; id < producer_count; ++id) {
        producers.emplace_back([&, id] {
            for (int i = 0; i < per_producer; ++i) concurrent_queue.push(id * per_producer + i);
        });
        consumers.emplace_back([&] {
            for (int i = 0; i < per_producer; ++i) {
                const int value = concurrent_queue.pop();
                assert(value >= 0 && value < producer_count * per_producer);
                seen[value].fetch_add(1);
            }
        });
    }
    for (auto& thread : producers) thread.join();
    for (auto& thread : consumers) thread.join();
    for (const auto& count : seen) assert(count.load() == 1);
}
