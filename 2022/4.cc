#include "common.h"

namespace aoc_2022_4 {

void run(std::string_view buf)
{
    int a0, a1, b0, b1;
    int part1 = 0;
    int part2 = 0;
    std::vector<std::string_view> tokens;

    for (std::string_view line : split_lines(buf)) {
        split(line, tokens, [](char c) { return c == ',' || c == '-'; });
        std::from_chars(tokens[0].begin(), tokens[0].end(), a0);
        std::from_chars(tokens[1].begin(), tokens[1].end(), a1);
        std::from_chars(tokens[2].begin(), tokens[2].end(), b0);
        std::from_chars(tokens[3].begin(), tokens[3].end(), b1);
        part1 += (a0 >= b0 && a1 <= b1) || (b0 >= a0 && b1 <= a1);
        part2 += a0 <= b1 && b0 <= a1;
    }

    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}

}
