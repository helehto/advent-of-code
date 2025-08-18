#include "common.h"

namespace aoc_2017_24 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    ASSERT(lines.size() < 64);

    inplace_vector<std::array<uint8_t, 2>, 64> components;
    for (std::string_view line : lines)
        components.unchecked_push_back(find_numbers_n<uint8_t, 2>(line));

    std::array<inplace_vector<std::pair<uint8_t, uint8_t>, 64>, 64> compatible;
    for (size_t i = 0; i < 64; ++i) {
        for (size_t j = 0; j < components.size(); ++j) {
            auto [a, b] = components[j];
            if (i == a)
                compatible[i].unchecked_emplace_back(j, b);
            if (i == b)
                compatible[i].unchecked_emplace_back(j, a);
        }
    }

    small_vector<std::tuple<uint64_t, int, int, int>, 64> queue;
    queue.emplace_back(UINT64_MAX >> (64 - components.size()), 0, 0, 0);

    int part1 = INT_MIN;
    std::pair<int, int> part2{INT_MIN, INT_MIN};
    while (!queue.empty()) {
        auto [remaining, curr, depth, weight] = queue.back();
        queue.pop_back();

        weight += curr;

        part1 = std::max(part1, weight);
        part2 = std::max(part2, std::pair(depth, weight));

        for (const auto &[idx, other] : compatible[curr]) {
            const uint64_t bit = UINT64_C(1) << idx;
            if (remaining & bit)
                queue.emplace_back(remaining & ~bit, other, depth + 1, weight + curr);
        }
    }

    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2.second);
}

}
