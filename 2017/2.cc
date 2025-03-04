#include "common.h"

namespace aoc_2017_2 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    std::vector<int> nums;

    int s1 = 0;
    int s2 = 0;
    for (std::string_view line : lines) {
        find_numbers(line, nums);
        auto [min, max] = std::ranges::minmax_element(nums);
        s1 += *max - *min;

        for (size_t i = 0; i < nums.size(); ++i) {
            for (size_t j = i + 1; j < nums.size(); ++j) {
                auto a = nums[i];
                auto b = nums[j];
                if (a % b == 0) {
                    s2 += a / b;
                } else if (b % a == 0) {
                    s2 += b / a;
                }
            }
        }
    }
    fmt::print("{}\n", s1);
    fmt::print("{}\n", s2);
}

}
