#include "common.h"
#include "dense_set.h"

namespace aoc_2024_7 {

/// Concatenates the base 10 representations of `a` and `b`. For instance,
/// `concatenate(42, 152)` will return 42152.
constexpr inline uint64_t concatenate(uint64_t a, uint64_t b)
{
    return a * pow10i[digit_count_base10(b)] + b;
}

template <int Part>
constexpr bool solve(int64_t goal, const int64_t *operands, size_t n, int64_t accum = 0)
{
    if (n == 0)
        return accum == goal;

    auto a = operands[0];
    return solve<Part>(goal, operands + 1, n - 1, accum + a) ||
           solve<Part>(goal, operands + 1, n - 1, accum * a) ||
           (Part == 2 && solve<Part>(goal, operands + 1, n - 1, concatenate(accum, a)));
}

void run(std::string_view buf)
{
    std::vector<int64_t> nums;

    int64_t s1 = 0;
    int64_t s2 = 0;
    for (std::string_view line : split_lines(buf)) {
        find_numbers(line, nums);
        s1 += solve<1>(nums[0], &nums[1], nums.size() - 1) ? nums[0] : 0;
        s2 += solve<2>(nums[0], &nums[1], nums.size() - 1) ? nums[0] : 0;
    }

    fmt::print("{}\n", s1);
    fmt::print("{}\n", s2);
}

}
