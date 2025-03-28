#include "common.h"

namespace aoc_2015_2 {

void run(std::string_view buf)
{
    std::vector<std::array<int, 3>> input;

    for (std::string_view line : split_lines(buf))
        input.push_back(find_numbers_n<int, 3>(line));

    int part1 = 0;
    int part2 = 0;
    for (auto [l, w, h] : input) {
        part1 += 2 * l * w + 2 * w * h + 2 * h * l + std::min({l * w, w * h, h * l});
        std::array<int, 3> a = {{l, w, h}};
        std::sort(begin(a), end(a));
        part2 += 2 * a[0] + 2 * a[1] + l * w * h;
    }
    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}

}
