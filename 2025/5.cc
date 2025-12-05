#include "common.h"

namespace aoc_2025_5 {

void run(std::string_view buf)
{
    const auto lines = split_lines(buf);

    auto nl = std::ranges::find_if(lines, λa(a.empty()));
    ASSERT(nl != lines.end());
    const size_t num_ranges = nl - lines.begin();

    // Parse input:
    const auto nums = find_numbers<uint64_t>(buf);
    std::vector<std::pair<uint64_t, uint64_t>> ranges;
    std::span<const uint64_t> ingredients;
    ranges.reserve(num_ranges);
    {
        size_t i = 0;
        for (; i < 2 * num_ranges; i += 2)
            ranges.emplace_back(nums[i], nums[i + 1] + 1);
        ingredients = {nums.begin() + i, nums.end()};
    }

    // Merge intervals:
    std::ranges::sort(ranges);
    std::vector<std::pair<uint64_t, uint64_t>> merged;
    merged.reserve(ranges.size());
    merged.push_back(ranges[0]);
    for (size_t i = 1; i < ranges.size(); ++i) {
        if (auto &end = merged.back().second; ranges[i].first <= end)
            end = std::max(end, ranges[i].second);
        else
            merged.emplace_back(ranges[i]);
    }

    // Part 1:
    uint64_t available_fresh = 0;
    for (const auto ing : ingredients) {
        auto it = std::ranges::upper_bound(merged, ing, {}, λa(a.second));
        available_fresh += (it != merged.end() && it->first <= ing && ing < it->second);
    }
    fmt::print("{}\n", available_fresh);

    // Part 2:
    uint64_t total_fresh = 0;
    for (const auto &[a, b] : merged)
        total_fresh += b - a;
    fmt::print("{}\n", total_fresh);
}

}
