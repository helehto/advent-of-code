#include "common.h"

namespace aoc_2021_22 {

using Region = std::array<std::array<int, 2>, 3>;

constexpr bool overlaps(const Region &a, const Region &b)
{
    return !((a[0][1] <= b[0][0] || a[0][0] >= b[0][1]) ||
             (a[1][1] <= b[1][0] || a[1][0] >= b[1][1]) ||
             (a[2][1] <= b[2][0] || a[2][0] >= b[2][1]));
}

constexpr std::optional<Region> intersect(const Region &a, const Region &b)
{
    if (!overlaps(a, b))
        return std::nullopt;

    return Region{{
        {std::max(a[0][0], b[0][0]), std::min(a[0][1], b[0][1])},
        {std::max(a[1][0], b[1][0]), std::min(a[1][1], b[1][1])},
        {std::max(a[2][0], b[2][0]), std::min(a[2][1], b[2][1])},
    }};
}

constexpr int64_t volume(const Region &a)
{
    return (static_cast<int64_t>(a[0][1]) - a[0][0]) *
           (static_cast<int64_t>(a[1][1]) - a[1][0]) *
           (static_cast<int64_t>(a[2][1]) - a[2][0]);
}

static int64_t count(std::span<const std::pair<bool, Region>> input)
{
    if (input.empty())
        return 0;

    const auto [state, region] = input.front();
    const auto rest = input.subspan(1);
    if (!state)
        return count(rest);

    small_vector<std::pair<bool, Region>, 8> overlaps;
    for (auto [_, r] : rest)
        if (auto ix = intersect(region, r))
            overlaps.emplace_back(true, *ix);

    return volume(region) + count(rest) - count(overlaps);
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    small_vector<std::pair<bool, Region>> full_input;
    for (std::string_view line : lines) {
        auto [x0, x1, y0, y1, z0, z1] = find_numbers_n<int, 6>(line);
        full_input.emplace_back(line.substr(0, 2) == "on",
                                Region{{{x0, x1 + 1}, {y0, y1 + 1}, {z0, z1 + 1}}});
    }

    constexpr Region part1_region{{{-50, 51}, {-50, 51}, {-50, 51}}};
    small_vector<std::pair<bool, Region>, 32> small_input;
    std::ranges::copy_if(full_input, std::back_inserter(small_input),
                         λa(overlaps(a.second, part1_region)));

    fmt::print("{}\n", count(small_input));
    fmt::print("{}\n", count(full_input));
}

}
