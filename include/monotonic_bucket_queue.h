#pragma once

#include "common.h"

/// An implementation of a priority queue in terms of a fixed-size bucket queue
/// (see <https://en.wikipedia.org/wiki/Bucket_queue>).
template <typename T>
class MonotonicBucketQueue {
    std::vector<std::vector<T>> buckets;
    uint32_t curr = 0;
    uint32_t mask;

public:
    constexpr MonotonicBucketQueue() = default;

    constexpr MonotonicBucketQueue(uint32_t max_weight)
        : mask(std::bit_ceil(max_weight + 1) - 1)
    {
        buckets.resize(mask + 1);
    }

    /// Return the current priority, i.e. the priority of the last element
    /// fetched via pop(), or 0 if no element has been removed yet.
    constexpr uint32_t current_priority() const { return curr; }

    template <typename... Args>
    constexpr void emplace(const uint32_t priority, Args &&...args)
    {
        DEBUG_ASSERT_MSG(priority >= curr,
                         "Priority {} is less than current priority {}!", priority, curr);
        DEBUG_ASSERT_MSG(priority <= curr + mask,
                         "Priority {} is greater than the current maximum {}!", priority,
                         curr + mask);
        buckets[priority & mask].emplace_back(std::forward<Args>(args)...);
    }

    /// Remove and return the lowest priority element from the queue.
    constexpr std::optional<T> pop()
    {
        for (size_t end = curr + mask + 1; curr < end; ++curr) {
            if (std::vector<T> &bucket = buckets[curr & mask]; !bucket.empty()) {
                T state(std::move(bucket.back()));
                bucket.pop_back();
                return state;
            }
        }
        return std::nullopt;
    }

    /// Clear the queue to an empty state.
    constexpr void clear()
    {
        for (std::vector<T> &bucket : buckets)
            bucket.clear();
        curr = 0;
    }

    /// Clear the queue and reconfigure it to a new maximum weight.
    constexpr void reset(size_t new_max_weight)
    {
        clear();
        mask = std::bit_ceil(new_max_weight + 1) - 1;
        buckets.resize(mask + 1);
    }
};
