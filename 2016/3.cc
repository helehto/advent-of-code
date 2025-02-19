#include "common.h"

namespace aoc_2016_3 {

static std::array<int, 3> sort3(int a, int b, int c)
{
    if (a > c)
        std::swap(a, c);
    if (a > b)
        std::swap(a, b);
    if (b > c)
        std::swap(b, c);

    return {a, b, c};
}

static int part1(const std::vector<int> &nums)
{
    int s = 0;

    for (size_t i = 0; i + 2 < nums.size(); i += 3) {
        auto [a, b, c] = sort3(nums[i], nums[i + 1], nums[i + 2]);
        s += a + b > c;
    }

    return s;
}

static int part2(const std::vector<int> &nums)
{
    int s = 0;

    for (size_t j = 0; j + 8 < nums.size(); j += 9) {
        auto [a, b, c] = sort3(nums[j + 0], nums[j + 3], nums[j + 6]);
        auto [d, e, f] = sort3(nums[j + 1], nums[j + 4], nums[j + 7]);
        auto [g, h, i] = sort3(nums[j + 2], nums[j + 5], nums[j + 8]);
        s += (a + b > c) + (d + e > f) + (g + h > i);
    }

    return s;
}

void run(std::string_view buf)
{
    auto nums = find_numbers<int>(buf);
    ASSERT(nums.size() % 3 == 0);
    fmt::print("{}\n", part1(nums));
    fmt::print("{}\n", part2(nums));
}

}
