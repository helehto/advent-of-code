#include "common.h"
#include "md5.h"
#include "thread_pool.h"
#include <hwy/highway.h>

namespace aoc_2015_4 {

/// 0000-9999 packed into a single string, plus a few extra entries wrapping
/// around to 0000 to avoid bounds checks in hash_search().
constexpr auto digits_4x = [] consteval {
    std::array<char, 4 * (10000 + md5::max_lanes)> table;
    for (size_t i = 0; 4 * i < table.size(); ++i) {
        table[4 * i + 0] = '0' + i / 1000;
        table[4 * i + 1] = '0' + (i / 100) % 10;
        table[4 * i + 2] = '0' + (i / 10) % 10;
        table[4 * i + 3] = '0' + i % 10;
    }
    return table;
}();

static void hash_search(std::string_view prefix,
                        std::atomic_uint64_t &next_chunk,
                        std::atomic_uint64_t &part1,
                        std::atomic_uint64_t &part2)
{
    const size_t lanes = md5::lanes();

    // Fill in the string prefix (the problem input) in each message block.
    // This will stay intact across all iterations below.
    md5::SequentialBlocks messages{};
    for (size_t i = 0; i < lanes; ++i)
        std::ranges::copy(prefix, &messages.data[i * md5::bytes_per_block]);

    auto suffix_ptr = [&](size_t msg) -> char * {
        DEBUG_ASSERT(msg < lanes);
        return messages.data + md5::bytes_per_block * msg + prefix.size();
    };

    while (part2.load(std::memory_order_relaxed) == INT64_MAX) {
        constexpr uint64_t chunk_size = 10000;

        // Reserve a chunk of suffixes to search.
        const uint64_t chunk_start =
            next_chunk.fetch_add(chunk_size, std::memory_order_relaxed);
        DEBUG_ASSERT(chunk_start % chunk_size == 0);

        const size_t suffix_len = digit_count_base10(chunk_start);

        // Prepare the messages, writing out everything needed to hash them
        // except for the last four digits of each suffix.
        std::array<uint32_t, md5::max_lanes> lengths;
        lengths.fill(prefix.size() + suffix_len);
        prepare_final_blocks(messages, lengths.data());
        for (size_t i = 0; i < lanes; i++)
            md5::to_chars(suffix_ptr(i), chunk_start + i);

        // Since we can write four digits at a time using a single 4-byte
        // store, this inner loop updates only the last four digits of each
        // message.
        for (uint64_t tail = 0; tail < chunk_size; tail += lanes) {
            // We tolerate `tail` being slightly larger than 9999 here since
            // the digits_4x table contains a few extra entries wrapping around
            // to 0000.
            for (size_t i = 0; i < lanes; i++)
                memcpy(suffix_ptr(i) + suffix_len - 4, &digits_4x[4 * (tail + i)], 4);

            // NOTE: This still has to interleave the message blocks for each
            // batch of messages to hash. This is likely the one remaining
            // thing that would yield a non-trivial speedup if eliminated; it
            // accounts for ~20% of the total runtime on my machine.
            //
            // But in that case, all logic in this function would have to deal
            // with the suffix potentially being split into 4-byte words at
            // variable offsets depending on the prefix length, instead of
            // being contiguous in each message. Thinking about that gives me a
            // headache, so let's just not.
            const md5::InterleavedBlocks blocks = md5::interleave(messages);
            const md5::VecT hashes = hn::Get4<0>(md5::hash_block(blocks));

            if (uint32_t m5 = md5::leading_zero_mask<5>(hashes)) [[unlikely]] {
                atomic_store_min(part1, chunk_start + tail + std::countr_zero(m5));

                if (uint32_t m6 = md5::leading_zero_mask<6>(hashes)) [[unlikely]] {
                    atomic_store_min(part2, chunk_start + tail + std::countr_zero(m6));
                    return;
                }
            }
        }
    }
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
