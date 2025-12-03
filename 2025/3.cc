#include "common.h"
#include "thread_pool.h"

namespace aoc_2025_3 {

constexpr std::pair<size_t, size_t>
scan_largest(std::string_view s, size_t left, size_t start = 0)
{
    size_t index = SIZE_MAX;
    size_t digit = 0;
    for (size_t i = start; i <= s.size() - left; ++i) {
        if (size_t d = s[i] - '0'; d > digit) {
            index = i;
            digit = d;
        }
    }

    return {index, digit};
}

constexpr size_t max_joltage(std::string_view s, size_t n)
{
    size_t d, idx = SIZE_MAX;
    size_t result = 0;
    for (size_t i = 0; i < n; ++i) {
        // TODO: Finding the largest digit can be done faster. (DP or SIMD?)
        std::tie(idx, d) = scan_largest(s, n - i, idx + 1);
        result = 10 * result + d;
    }
    return result;
}

void run(std::string_view buf)
{
    std::vector<std::string_view> lines = split_lines(buf);
    ThreadPool &pool = ThreadPool::get();
    std::atomic<uint64_t> part1 = 0;
    std::atomic<uint64_t> part2 = 0;

    pool.for_each_index(0, lines.size(), [&](size_t begin, size_t end) {
        uint64_t s1 = 0;
        uint64_t s2 = 0;
        for (size_t k = begin; k < end; ++k) {
            s1 += max_joltage(lines[k], 2);
            s2 += max_joltage(lines[k], 12);
        }
        part1.fetch_add(s1, std::memory_order_relaxed);
        part2.fetch_add(s2, std::memory_order_relaxed);
    });

    fmt::print("{}\n{}\n", part1.load(), part2.load());
}

}
