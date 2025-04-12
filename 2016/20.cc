#include "common.h"

namespace aoc_2016_20 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::vector<std::pair<uint32_t, uint32_t>> unmerged;
    unmerged.reserve(lines.size());
    for (std::string_view line : lines) {
        auto [a, b] = find_numbers_n<uint32_t, 2>(line);
        unmerged.emplace_back(a, b);
    }

    std::ranges::sort(unmerged, {}, λx(x.first));

    std::vector<std::pair<uint32_t, uint32_t>> merged;
    merged.reserve(unmerged.size());
    merged.push_back(unmerged[0]);

    for (size_t i = 1; i < unmerged.size(); ++i) {
        if (auto &end = merged.back().second; unmerged[i].first <= end)
            end = std::max(end, unmerged[i].second);
        else
            merged.emplace_back(unmerged[i]);
    }

    uint32_t blocked = 0;
    for (const auto &[a, b] : merged)
        blocked += b - a + 1;

    fmt::print("{}\n{}\n", merged[0].second + 1, UINT32_MAX - blocked + 1);
}

}
