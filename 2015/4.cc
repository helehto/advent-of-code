#include "common.h"
#include "md5.h"
#include "thread_pool.h"
#include <hwy/highway.h>

namespace aoc_2015_4 {

static void hash_search(std::string_view prefix,
                        std::atomic_uint64_t &next_chunk,
                        std::atomic_uint64_t &part1,
                        std::atomic_uint64_t &part2)
{
    auto sink = [&](md5::VecT hashes, uint64_t n) {
        uint32_t m5 = md5::leading_zero_mask<5>(hashes);
        if (m5 == 0) [[likely]]
            return true;
        atomic_store_min(part1, n + std::countr_zero(m5));

        uint32_t m6 = md5::leading_zero_mask<6>(hashes);
        if (m6 == 0) [[likely]]
            return true;
        atomic_store_min(part2, n + std::countr_zero(m6));

        // Found both, stop searching.
        return false;
    };

    auto messages = md5::SequentialBlocks::splat(prefix);
    uint64_t chunk_start;
    do {
        chunk_start = next_chunk.fetch_add(10000, std::memory_order_relaxed);
    } while (part2.load(std::memory_order_relaxed) == INT64_MAX &&
             hash_4digit_chunks(messages, chunk_start, prefix.size(), sink));
}

void run(std::string_view buf)
{
    ThreadPool &pool = ThreadPool::get();
    alignas(64) std::atomic_uint64_t part1 = INT64_MAX;
    alignas(64) std::atomic_uint64_t part2 = INT64_MAX;
    alignas(64) std::atomic_uint64_t next_chunk = 0;
    pool.for_each_thread([&](int64_t) { hash_search(buf, next_chunk, part1, part2); });
    fmt::print("{}\n", part1.load());
    fmt::print("{}\n", part2.load());
}

}
