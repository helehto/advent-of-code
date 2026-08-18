#include <algorithm>
#include <aoc/base.h>
#include <aoc/macros.h>
#include <aoc/string.h>
#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

namespace aoc_2025_12 {

void run(std::string_view buf, aoc::Answer &answer)
{
    const auto lines = split_lines(buf);

    int solution = 0;
    std::vector<int> nums;
    for (size_t i = lines.size() - 1; !lines[i].empty(); --i) {
        find_numbers(lines[i], nums);
        const auto w = nums[0];
        const auto h = nums[1];
        const auto count = std::span(nums).subspan(2);
        const auto need_space = std::ranges::fold_left(count, 0, λab(a + b));
        solution += need_space <= (w / 3) * (h / 3);
    }

    answer.add(solution);
}
AOC_REGISTER_SOLVER(2025, 12, run);

}
