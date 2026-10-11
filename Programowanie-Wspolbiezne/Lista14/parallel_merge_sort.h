#pragma once

#include "concurrent_task_engine.h"

#include <algorithm>
#include <exception>
#include <functional>
#include <iterator>
#include <type_traits>
#include <vector>

template <typename RandomIterator, typename Engine = ConcurrentTaskEngine, typename Compare = std::less<>>
void parallel_merge_sort(RandomIterator first, RandomIterator last,
                         Engine& engine, Compare compare = Compare{}) {
    using difference_type = typename std::iterator_traits<RandomIterator>::difference_type;
    using value_type = typename std::iterator_traits<RandomIterator>::value_type;
    const difference_type count = last - first;
    if (count < 2) return;

    // vector<bool> packs adjacent values into shared storage words.
    // Sequential sorting avoids concurrent writes to those words.
    if constexpr (std::is_same_v<value_type, bool>) {
        std::stable_sort(first, last, compare);
        return;
    }

    std::vector<value_type> buffer(static_cast<std::size_t>(count));
    bool input_is_original = true;
    for (difference_type width = 1; width < count; width *= 2) {
        std::vector<std::future<void>> jobs;
        const difference_type block_width = width > count / 2 ? count : width * 2;
        jobs.reserve(static_cast<std::size_t>((count - 1) / block_width + 1));
        std::exception_ptr failure;
        try {
            for (difference_type begin = 0; begin < count; begin += block_width) {
                const difference_type middle = begin + std::min(width, count - begin);
                const difference_type end = begin + std::min(block_width, count - begin);
                jobs.push_back(engine.submit([=, &buffer] {
                    if (input_is_original) {
                        std::merge(first + begin, first + middle, first + middle, first + end,
                                   buffer.begin() + begin, compare);
                    } else {
                        std::merge(buffer.begin() + begin, buffer.begin() + middle,
                                   buffer.begin() + middle, buffer.begin() + end,
                                   first + begin, compare);
                    }
                }));
                if (end == count) break;
            }
        } catch (...) {
            failure = std::current_exception();
        }
        for (auto& job : jobs) {
            try {
                job.get();
            } catch (...) {
                if (!failure) failure = std::current_exception();
            }
        }
        if (failure) std::rethrow_exception(failure);
        input_is_original = !input_is_original;
        if (width > count / 2) break;
    }
    if (!input_is_original) std::move(buffer.begin(), buffer.end(), first);
}
