#include "common.h"

namespace aoc_2022_20 {

constexpr int index_shift = 16;

static void mix(const std::vector<int64_t> &original, std::vector<int64_t> &nums)
{
    int64_t *data = nums.data();
    const size_t size = nums.size();

    // Who needs linked lists when you have an L1 cache and a prefetcher?
    for (const auto x : original) {
        const size_t i = std::find(data, data + size, x) - data;
        const size_t j = modulo<int64_t>(i + (x >> index_shift), nums.size() - 1);

        if (i < j)
            memmove(data + i, data + i + 1, (j + 1 - i) * sizeof(nums[0]));
        else
            memmove(data + j + 1, data + j, (i - j) * sizeof(nums[0]));

        nums[j] = x;
    }
}

static int64_t solve(std::vector<int64_t> nums, int key, int rounds, int64_t zero)
{
    for (size_t i = 0; i < nums.size(); i++)
        nums[i] = ((nums[i] >> index_shift) * key) << index_shift | i;

    const std::vector<int64_t> original(nums);
    for (int i = 0; i < rounds; i++)
        mix(original, nums);

    const auto zero_index = std::ranges::find(nums, zero) - nums.begin();
    return (nums[(zero_index + 1000) % nums.size()] >> index_shift) +
           (nums[(zero_index + 2000) % nums.size()] >> index_shift) +
           (nums[(zero_index + 3000) % nums.size()] >> index_shift);
}

void run(std::string_view buf)
{
    std::vector<int64_t> nums = find_numbers<int64_t>(buf);

    int64_t zero = 0;
    for (int i = 0; int64_t &n : nums) {
        // To disambiguate duplicates, pack the index into the lower 16 bits
        // and the original number into the high 48 bits. This way we don't
        // need to keep track of a separate index array.
        if (n == 0)
            zero = i;
        n = n << index_shift | i;
        i++;
    }

    fmt::print("{}\n", solve(nums, 1, 1, zero));
    fmt::print("{}\n", solve(nums, 811589153, 10, zero));
}

}
