#include <atomic>
#include <barrier>
#include <cassert>
#include <optional>
#include <thread>
#include <vector>

#include "lock_free_stack_ibr.h"

int main() {
    {
        lock_free_stack_ibr<int> stack(1);
        assert(!stack.pop().has_value());

        stack.push(10);
        stack.push(20);

        assert(stack.pop() == std::optional<int>{20});
        assert(stack.pop() == std::optional<int>{10});
        assert(!stack.pop().has_value());
    }

    {
        constexpr int producer_count = 4;
        constexpr int consumer_count = 4;
        constexpr int values_per_producer = 500;
        constexpr int total = producer_count * values_per_producer;

        lock_free_stack_ibr<int> stack(producer_count + consumer_count + 1);
        std::barrier start(producer_count + consumer_count + 1);
        std::atomic<int> finished_producers{0};
        std::vector<std::vector<int>> consumed(consumer_count);

        {
            std::vector<std::jthread> workers;
            workers.reserve(producer_count + consumer_count);

            for (int producer = 0; producer < producer_count; ++producer) {
                workers.emplace_back([&, producer] {
                    start.arrive_and_wait();
                    for (int value = 0; value < values_per_producer; ++value) {
                        stack.push(producer * values_per_producer + value);
                    }
                    finished_producers.fetch_add(1, std::memory_order_release);
                });
            }

            for (int consumer = 0; consumer < consumer_count; ++consumer) {
                workers.emplace_back([&, consumer] {
                    start.arrive_and_wait();
                    while (true) {
                        if (auto value = stack.pop()) {
                            consumed[consumer].push_back(*value);
                        } else if (finished_producers.load(std::memory_order_acquire) ==
                                   producer_count) {
                            return;
                        } else {
                            std::this_thread::yield();
                        }
                    }
                });
            }

            start.arrive_and_wait();
        }

        std::vector<bool> seen(total, false);
        int received_count = 0;
        for (const auto& values : consumed) {
            for (const int value : values) {
                assert(value >= 0 && value < total);
                assert(!seen[value]);
                seen[value] = true;
                ++received_count;
            }
        }

        assert(received_count == total);
        assert(!stack.pop().has_value());
    }
}
