#include <aoc/base.h>
#include <aoc/math.h>
#include <aoc/string.h>
#include <charconv>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace aoc_2020_13 {

void run(std::string_view buf, aoc::Answer &answer)
{
    auto lines = split_lines(buf);

    int depart_after = 0;
    std::from_chars(lines[0].data(), lines[0].data() + lines[0].size(), depart_after);

    std::vector<std::string_view> fields;
    split(lines[1], fields, ',');

    std::vector<int> nums(fields.size(), -1);
    for (size_t i = 0; i < fields.size(); ++i)
        std::from_chars(fields[i].data(), fields[i].data() + fields[i].size(), nums[i]);

    // Part 1:
    int earliest_bus = 0;
    int min_wait = INT_MAX;
    for (size_t i = 0; i < nums.size(); ++i) {
        if (nums[i] > 0) {
            if (int wait = nums[i] - depart_after % nums[i]; wait < min_wait) {
                earliest_bus = i;
                min_wait = wait;
            }
        }
    }
    answer.add(nums[earliest_bus] * min_wait);

    // Part 2, using the Chinese remainder theorem:
    int64_t M = 1;
    for (int64_t n : nums)
        if (n > 0)
            M *= n;
    int64_t x = 0;
    for (size_t i = 0; i < nums.size(); ++i) {
        if (nums[i] > 0) {
            int64_t a = nums[i] - i;
            int64_t b = M / nums[i];
            x += a * b * modinv(b, nums[i]);
        }
    }
    answer.add(x % M);
}
AOC_REGISTER_SOLVER(2020, 13, run);

}
