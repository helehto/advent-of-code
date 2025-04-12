#include "common.h"
#include <cinttypes>

namespace aoc_2022_3 {

static uint64_t make_mask(std::string_view s)
{
    uint64_t mask = 0;

    for (auto c : s) {
        if (c >= 'a')
            mask |= UINT64_C(1) << (c - 'a');
        else
            mask |= UINT64_C(1) << (c - 'A' + 26);
    }
    return mask;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    int part1 = 0;
    for (const auto &s : lines) {
        auto mid = s.begin() + s.size() / 2;
        const auto mask1 = make_mask({s.begin(), mid});
        const auto mask2 = make_mask({mid, s.end()});
        part1 += std::countr_zero(mask1 & mask2) + 1;
    }

    int part2 = 0;
    for (size_t i = 0; i < lines.size(); i += 3) {
        const auto mask1 = make_mask(lines[i]);
        const auto mask2 = make_mask(lines[i + 1]);
        const auto mask3 = make_mask(lines[i + 2]);
        part2 += std::countr_zero(mask1 & mask2 & mask3) + 1;
    }

    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}

}
