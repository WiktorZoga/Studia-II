#include "resource_manager.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <latch>
#include <thread>

using namespace std::chrono_literals;

namespace {

void test_search_and_insert_overlap_but_inserts_serialize() {
    ResourceManager manager;
    std::latch insert_started{1};
    std::latch release_insert{1};
    std::atomic<int> active_inserters{0};
    std::atomic<int> maximum_inserters{0};

    manager.start_search();
    std::thread inserter([&] {
        manager.start_insert();
        const int current = active_inserters.fetch_add(1) + 1;
        maximum_inserters.store(current);
        insert_started.count_down();
        release_insert.wait();
        active_inserters.fetch_sub(1);
        manager.end_insert();
    });

    insert_started.wait();
    assert(maximum_inserters.load() == 1);

    std::atomic<bool> second_inserted{false};
    std::thread second_inserter([&] {
        manager.start_insert();
        second_inserted.store(true);
        manager.end_insert();
    });

    std::this_thread::sleep_for(20ms);
    assert(!second_inserted.load());
    release_insert.count_down();
    inserter.join();
    second_inserter.join();
    manager.end_search();
    assert(second_inserted.load());
}

void test_delete_is_exclusive_and_waiting_delete_blocks_new_searches() {
    ResourceManager manager;
    manager.start_search();

    std::atomic<bool> delete_entered{false};
    std::atomic<bool> late_search_entered{false};
    std::latch release_delete{1};

    std::thread deleter([&] {
        manager.start_delete();
        delete_entered.store(true);
        release_delete.wait();
        manager.end_delete();
    });

    std::this_thread::sleep_for(20ms);
    std::thread late_search([&] {
        manager.start_search();
        late_search_entered.store(true);
        manager.end_search();
    });

    std::this_thread::sleep_for(20ms);
    assert(!delete_entered.load());
    assert(!late_search_entered.load());

    manager.end_search();
    while (!delete_entered.load()) {
        std::this_thread::yield();
    }
    assert(!late_search_entered.load());

    release_delete.count_down();
    deleter.join();
    late_search.join();
    assert(late_search_entered.load());
}

} // namespace

int main() {
    test_search_and_insert_overlap_but_inserts_serialize();
    test_delete_is_exclusive_and_waiting_delete_blocks_new_searches();
}
