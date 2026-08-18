#include <algorithm>
#include <aoc/base.h>
#include <aoc/dense_map.h>
#include <aoc/dense_set.h>
#include <aoc/string.h>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace aoc_2020_1 {

void run(std::string_view buf, aoc::Answer &answer)
{
    std::vector<int> nums = find_numbers<int>(buf);

    dense_set<int> seen;
    for (const int num : nums) {
        if (seen.count(2020 - num)) {
            answer.add(num * (2020 - num));
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
            answer.add(a * b * c);
            break;
        }
    }
}
AOC_REGISTER_SOLVER(2020, 1, run);

}
