#include "common.h"
#include "dense_set.h"

namespace aoc_2017_5 {

static int part1(std::vector<uint64_t> nums)
{
    size_t steps = 0;
    for (uint64_t i = 0; i < nums.size(); steps++)
        i += nums[i]++;
    return steps;
}

[[gnu::noinline]] static int part2(std::vector<uint64_t> nums)
{
    size_t steps = 0;
    uint64_t *p = &nums[0];
    uint64_t *end = p + nums.size();
    while (true) {
        // The loop-carried dependency due to the pointer `p` is the bottleneck
        // here, and it appears to be more or less impossible to get rid of
        // without starting to assume things about the input. The only way to
        // optimize this is to minimize the latency for each loop iteration.
        //
        // This seems to work fastest on my setup (Zen 4): use an ordinary load
        // (4 cycles) rather than a load with sign extension (5 cycles), and
        // increment/decrement the new instruction with a sequence which does
        // not involve a branch or a conditional move. 64-bit integers are used
        // to avoid needing to zero-extend the index in each iteration (cache
        // misses are a non-factor since the vector is only 8 KiB or so).
        //
        // Despite the dependency on `p`, the core loop somehow is ~5% faster
        // by having the compiler unroll it by 4x. It reduces the CPU counters
        // ex_no_retire.load_not_complete and ex_no_retire.load_not_complete
        // reported by 'perf stat' by >50% compared to no unrolling at all, but
        // I have no idea why; each iteration, unrolled or not, has more or
        // less the same structure with regard to memory loads.
        for (size_t i = 0; i < 4; ++i) {
            const uint64_t instr = *p;
            *p = instr - ((static_cast<int64_t>(instr - 3) >> 63) | 1);
            p += instr;
            steps++;
            if (p >= end) [[unlikely]]
                return steps;
        }
    }
    return steps;
}

void run(std::string_view buf)
{
    auto nums = find_numbers<int64_t>(buf);
    std::vector<uint64_t> nums2;
    for (int64_t n : nums)
        nums2.push_back(static_cast<uint64_t>(n));

    fmt::print("{}\n", part1(nums2));
    fmt::print("{}\n", part2(std::move(nums2)));
}

}
