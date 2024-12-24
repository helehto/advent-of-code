#include "common.h"
#include "dense_map.h"

namespace aoc_2020_9 {

void run(FILE *f)
{
    constexpr size_t window_length = 25;
    const auto [buf, _] = slurp_lines(f);
    std::vector<uint64_t> nums;
    find_numbers(buf, nums);

    dense_map<uint64_t, uint8_t> sum_frequencies;
    sum_frequencies.reserve(window_length * window_length * window_length);

    // Compute sums of preamble:
    for (size_t i = 0; i < window_length; ++i)
        for (size_t j = i + 1; j < window_length; j++)
            sum_frequencies[nums[i] + nums[j]]++;

    // Part 1:
    size_t w = window_length;
    for (; w < nums.size() && sum_frequencies[nums[w]]; w++) {
        for (size_t j = w - window_length + 1; j < w; j++) {
            sum_frequencies[nums[w] + nums[j]]++;
            sum_frequencies[nums[w - window_length] + nums[j]]--;
        }
    }
    fmt::print("{}\n", nums[w]);

    // Part 2:
    size_t i = 0;
    size_t j = 0;
    uint64_t sum = 0;
    while (true) {
        if (sum < nums[w]) {
            sum += nums[j];
            j++;
        } else if (sum > nums[w]) {
            sum -= nums[i];
            i++;
        } else {
            break;
        }
    }
    auto [min, max] = std::minmax_element(nums.data() + i, nums.data() + j);
    fmt::print("{}\n", *min + *max);
}

}
