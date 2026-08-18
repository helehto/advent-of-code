#include <algorithm>
#include <aoc/base.h>
#include <aoc/macros.h>
#include <aoc/md5.h>
#include <aoc/thread_pool.h>
#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <hwy/highway.h>
#include <iterator>
#include <mutex>
#include <string>
#include <string_view>

namespace aoc_2016_5 {

// The character itself is stored in the low 8 bits, and the high 56 bits are
// the suffix that derived that character.
using Password = std::array<uint64_t, 8>;

static void add_part1_character(Password &p, uint64_t suffix, uint32_t c)
{
    const uint64_t packed = suffix << 8 | c;
    if (auto it = std::ranges::lower_bound(p, packed); it != end(p)) {
        std::move_backward(it, end(p) - 1, end(p));
        *it = packed;
    }
}

static void add_part2_character(Password &p, uint64_t suffix, size_t index, uint32_t c)
{
    if (index < 8)
        p[index] = std::min(p[index], suffix << 8 | c);
}

static void search(std::mutex &mutex,
                   Password &p1,
                   Password &p2,
                   std::string_view prefix,
                   std::atomic_uint64_t &next_chunk)
{
    auto sink = [&](md5::VecT hashes, uint64_t n) {
        for (auto m = md5::leading_zero_mask<5>(hashes); m; m &= m - 1) {
            const auto bit = std::countr_zero(m);
            const auto h1 = (ExtractLane(hashes, bit) >> 16) & 0xf;
            const auto h2 = (ExtractLane(hashes, bit) >> 28) & 0xf;
            std::unique_lock lk(mutex);
            add_part1_character(p1, n + bit, "0123456789abcdef"[h1]);
            add_part2_character(p2, n + bit, h1, "0123456789abcdef"[h2]);
        }
        return true;
    };

    auto done = [&](uint64_t suffix) {
        std::unique_lock lk(mutex);
        const bool done1 = (suffix << 8) >= p1.back();
        const bool done2 = std::ranges::all_of(p2, λa(a < UINT64_MAX));
        return done1 && done2;
    };

    auto messages = md5::SequentialBlocks::splat(prefix);
    uint64_t chunk_start;
    do {
        chunk_start = next_chunk.fetch_add(10000, std::memory_order_relaxed);
        hash_4digit_chunks(messages, chunk_start, prefix.size(), sink);
    } while (!done(chunk_start));
}

void run(std::string_view buf, aoc::Answer &answer)
{
    std::atomic_uint64_t next_chunk = 0;
    std::mutex mutex;
    Password p1, p2;
    p1.fill(UINT64_MAX);
    p2.fill(UINT64_MAX);

    ThreadPool::get().for_each_thread(
        [&](size_t) noexcept { search(mutex, p1, p2, buf, next_chunk); });

    answer.add(std::string(begin(p1), end(p1)));
    answer.add(std::string(begin(p2), end(p2)));
}
AOC_REGISTER_SOLVER(2016, 5, run);

}
