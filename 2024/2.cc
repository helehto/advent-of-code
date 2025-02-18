#include "common.h"

namespace aoc_2024_2 {

constexpr bool is_safe1(std::span<const int> nums)
{
    for (size_t i = 1; i < nums.size(); ++i) {
        int d = std::abs(nums[i] - nums[i - 1]);
        if (d < 1 || d > 3)
            return false;
    }
    return std::is_sorted(nums.begin(), nums.end()) ||
           std::is_sorted(nums.rbegin(), nums.rend());
}

constexpr bool is_safe2(std::span<const int> nums)
{
    std::vector<int> v(nums.begin(), nums.end());
    for (size_t i = nums.size(); i--;) {
        auto saved = v[i];
        v.erase(v.begin() + i);
        if (is_safe1(v))
            return true;
        v.insert(v.begin() + i, saved);
    }
    return false;
}

void run(std::string_view buf)
{
    int s1 = 0;
    int s2 = 0;
    std::vector<int> nums;
    for (std::string_view line : split_lines(buf)) {
        find_numbers(line, nums);
        s1 += is_safe1(nums);
        s2 += is_safe2(nums);
    }

    fmt::print("{}\n", s1);
    fmt::print("{}\n", s2);
}

}
