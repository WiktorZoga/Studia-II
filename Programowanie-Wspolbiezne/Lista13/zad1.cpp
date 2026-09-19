#include <cassert>
#include <barrier>
#include <atomic>
#include <optional>
#include <thread>
#include <vector>

#include "lock_free_stack.h"

int main() {

    {
        // Testy stosu dla jednego wątku

        lock_free_stack<int> stack(1);

        std::optional<int> empty_stack_pop = stack.pop();

        assert(!empty_stack_pop.has_value());

        stack.push(10);

        stack.push(20);

        std::optional<int> v1 = stack.pop();
        assert(v1.value() == 20);

        std::optional<int> v2 = stack.pop();
        assert(v2.value() == 10);

        empty_stack_pop = stack.pop();
        assert(!empty_stack_pop.has_value());
    }

    {
        // Testy dla wielu wątków, czy pushe nic nie gubią
        constexpr int worker_count = 8;
        constexpr int pushes_per_worker = 1'000;
        constexpr int total = worker_count * pushes_per_worker;

        lock_free_stack<int> stack(worker_count + 1);

        std::barrier start(worker_count + 1);

        {
            std::vector<std::jthread> workers;
            workers.reserve(worker_count);

            for (int worker = 0; worker < worker_count; worker++) {
                workers.emplace_back([&, worker] {
                    start.arrive_and_wait();
                    for (int i = 0; i < pushes_per_worker; i++) {
                        stack.push(worker * pushes_per_worker + i);
                    }
                });
            }

            start.arrive_and_wait();
        } // automatyczny join()

        std::vector<bool> seen(total, false);

        for (int i = 0; i < total; i++) {
            auto value = stack.pop();

            assert(value.has_value());
            assert(value.value() >= 0 && value.value() < total);
            assert(!seen[value.value()]);

            seen[value.value()] = true;
        }

        assert(!stack.pop().has_value());

    }

    {
        constexpr int producer_count = 8;
        constexpr int consumer_count = 8;
        constexpr int values_per_producer = 1'000;
        constexpr int total = producer_count * values_per_producer;

        lock_free_stack<int> stack(producer_count + consumer_count + 1);

        std::barrier start(producer_count + consumer_count + 1);
        std::atomic<int> finished_producers{0};

        std::vector<std::vector<int>> consumed(consumer_count);

        {
            std::vector<std::jthread> workers;
            workers.reserve(producer_count + consumer_count);

            for (int producer = 0; producer < producer_count; producer++) {
                workers.emplace_back([&, producer] {
                    start.arrive_and_wait();

                    for (int i = 0; i < values_per_producer; i++) {
                        stack.push(producer * values_per_producer + i);
                    }

                    finished_producers.fetch_add(1);
                });
            }

            for (int consumer = 0; consumer < consumer_count; consumer++) {
                workers.emplace_back([&, consumer] {
                    start.arrive_and_wait();

                    while (true) {
                        auto value = stack.pop();

                        if (value.has_value()) {
                            consumed[consumer].push_back(*value);
                        } else if (finished_producers.load() == producer_count) {
                            break;
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
            for (int value : values) {
                assert(value >= 0 && value < total);
                assert(!seen[value]);

                seen[value] = true;
                ++received_count;
            }
        }

        assert(received_count == total);
        assert(!stack.pop().has_value());
    }

    return 0;
}
