#include "common.h"

namespace aoc_2015_17 {

static int
part1(const std::vector<int> &containers, uint64_t used_mask, int left, size_t start)
{
    if (left == 0)
        return 1;

    int n = 0;

    for (size_t i = start; i < containers.size(); i++) {
        const uint64_t mask = (UINT64_C(1) << i);
        if (left >= containers[i] && (used_mask & mask) == 0)
            n += part1(containers, used_mask | mask, left - containers[i], i + 1);
    }

    return n;
}

static inline size_t next_bit_permutation(size_t v) noexcept
{
    const auto t = v | (v - 1);
    return (t + 1) | (((~t & -~t) - 1) >> (__builtin_ctzl(v) + 1));
}

static int capacity(const std::vector<int> &containers, uint64_t mask)
{
    int capacity = 0;

    for (; mask; mask &= mask - 1) {
        const auto bit = __builtin_ctzll(mask);
        capacity += containers[bit];
    }

    return capacity;
}

static int part2(const std::vector<int> &containers, int target)
{
    const uint64_t limit = UINT64_C(1) << containers.size();

    for (size_t i = 1; i <= containers.size(); i++) {
        int count = 0;
        for (uint64_t mask = (UINT64_C(1) << i) - 1; mask < limit;
             mask = next_bit_permutation(mask)) {
            count += (capacity(containers, mask) == target);
        }
        if (count != 0)
            return count;
    }

    return -1;
}

void run(std::string_view buf)
{
    auto containers = find_numbers<int>(buf);
    ASSERT(containers.size() < 64);

    fmt::print("{}\n", part1(containers, 0, 150, 0));
    fmt::print("{}\n", part2(containers, 150));
}

}
