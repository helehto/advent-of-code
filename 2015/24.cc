#include "common.h"

namespace aoc_2015_24 {

constexpr uint64_t subset_sum(std::span<const uint8_t> nums, uint64_t mask)
{
    uint64_t result = 0;
    for (; mask; mask &= mask - 1)
        result += nums[std::countr_zero(mask)];
    return result;
}

constexpr uint64_t subset_product(std::span<const uint8_t> nums, uint64_t mask)
{
    uint64_t result = 1;
    for (; mask; mask &= mask - 1)
        result *= nums[std::countr_zero(mask)];
    return result;
}

/// Compute the next lexicographic permutation of the bits in `v`.
///
/// Taken from <https://graphics.stanford.edu/~seander/bithacks.html>.
constexpr size_t next_bit_permutation(size_t v)
{
    const auto t = v | (v - 1);
    return (t + 1) | (((~t & -~t) - 1) >> (std::countr_zero(v) + 1));
}

constexpr uint64_t solve(std::span<const uint8_t> nums, uint64_t target)
{
    size_t n = 0;
    for (uint64_t suffix_sum = 0; suffix_sum < target; ++n)
        suffix_sum += nums[nums.size() - n - 1];

    std::optional<uint64_t> result_mask;
    const uint64_t K = UINT64_MAX >> (64 - nums.size());
    for (; !result_mask.has_value(); ++n) {
        for (uint64_t m = UINT64_MAX >> (64 - n); m <= K; m = next_bit_permutation(m)) {
            if (subset_sum(nums, m) == target &&
                (!result_mask.has_value() ||
                 subset_product(nums, m) < subset_product(nums, *result_mask))) {
                result_mask = m;
            }
        }
    }

    ASSERT(result_mask);
    return subset_product(nums, *result_mask);
}

void run(std::string_view buf)
{
    auto nums = find_numbers_small<uint8_t>(buf);
    ASSERT(nums.size() < 64);
    auto sum = std::ranges::fold_left(nums, 0, λxy(x + y));
    fmt::print("{}\n", solve(nums, sum / 3));
    fmt::print("{}\n", solve(nums, sum / 4));
}

}
