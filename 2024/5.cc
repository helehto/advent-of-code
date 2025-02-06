#include "common.h"
#include "dense_set.h"

namespace aoc_2024_5 {

void run(FILE *f)
{
    auto [_, lines] = slurp_lines(f);
    std::vector<int> nums;

    size_t i = 0;
    std::vector<uint8_t> ok(16384, false);
    for (; !lines[i].empty(); ++i) {
        find_numbers(lines[i], nums);
        ASSERT(nums[0] < 128);
        ASSERT(nums[1] < 128);
        ok[nums[0] << 7 | nums[1]] = true;
    }

    int n[2]{};
    for (++i; i < lines.size(); ++i) {
        find_numbers(lines[i], nums);

        bool incorrect_order = false;
        size_t end = nums.size();
        do {
            size_t new_end = 0;
            for (size_t i = 1; i < end; ++i) {
                if (!ok[nums[i - 1] << 7 | nums[i]]) {
                    std::swap(nums[i - 1], nums[i]);
                    incorrect_order = true;
                    new_end = i;
                }
            }
            end = new_end;
        } while (end);

        n[incorrect_order] += nums[nums.size() / 2];
    }

    fmt::print("{}\n{}\n", n[0], n[1]);
}

}
