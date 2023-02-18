#include "common.h"
#include <cinttypes>
#include <cstdint>
#include <fmt/core.h>
#include <string_view>
#include <vector>

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

void run_2022_3(FILE *f)
{
    std::vector<std::string> strings = getlines(f);

    int part1 = 0;
    for (const auto &s : strings) {
        auto mid = s.begin() + s.size() / 2;
        const auto mask1 = make_mask({s.begin(), mid});
        const auto mask2 = make_mask({mid, s.end()});
        part1 += __builtin_ctzll(mask1 & mask2) + 1;
    }

    int part2 = 0;
    for (size_t i = 0; i < strings.size(); i += 3) {
        const auto mask1 = make_mask(strings[i]);
        const auto mask2 = make_mask(strings[i + 1]);
        const auto mask3 = make_mask(strings[i + 2]);
        part2 += __builtin_ctzll(mask1 & mask2 & mask3) + 1;
    }

    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}
