#include "common.h"

namespace aoc_2017_24 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    std::vector<std::array<int, 2>> components;
    components.reserve(lines.size());

    for (std::string_view line : lines)
        components.push_back(find_numbers_n<int, 2>(line));
    ASSERT(components.size() <= 64);

    std::vector<std::tuple<uint64_t, int, int, int>> queue;
    queue.reserve(10'000);
    queue.emplace_back(~uint64_t(0) >> (64 - components.size()), 0, 0, 0);

    int part1 = INT_MIN;
    std::pair<int, int> part2{INT_MIN, INT_MIN};
    while (!queue.empty()) {
        auto [remaining_mask, curr, depth, weight] = queue.back();
        queue.pop_back();

        part1 = std::max(part1, weight);
        part2 = std::max(part2, std::pair(depth, weight));

        for (auto mask = remaining_mask; mask; mask &= mask - 1) {
            const auto [a, b] = components[std::countr_zero(mask)];
            const auto new_mask = remaining_mask & ~(mask & -mask);
            if (a == curr)
                queue.emplace_back(new_mask, b, depth + 1, weight + a + b);
            else if (b == curr)
                queue.emplace_back(new_mask, a, depth + 1, weight + a + b);
        }
    }

    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2.second);
}

}
