#include "common.h"
#include "dense_map.h"
#include "dense_set.h"
#include <vector>

namespace aoc_2020_1 {

void run(std::string_view buf)
{
    std::vector<int> nums = find_numbers<int>(buf);

    dense_set<int> seen;
    for (const int num : nums) {
        if (seen.count(2020 - num)) {
            fmt::print("{}\n", num * (2020 - num));
            break;
        }
        seen.insert(num);
    }

    std::ranges::sort(nums);

    dense_map<int, std::pair<uint16_t, uint16_t>> pairs;
    pairs.reserve(nums.size() * (nums.size() + 1) / 2);
    for (size_t i = 0; i < nums.size(); ++i) {
        for (size_t j = i; j < nums.size(); ++j) {
            pairs.emplace(nums[i] + nums[j],
                          std::pair<uint16_t, uint16_t>(nums[i], nums[j]));
        }
    }
    for (const int c : nums) {
        if (auto it = pairs.find(2020 - c); it != end(pairs)) {
            const auto [a, b] = it->second;
            fmt::print("{}\n", a * b * c);
            break;
        }
    }
}
}
