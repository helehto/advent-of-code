#include "common.h"
#include <algorithm>
#include <cinttypes>
#include <cstring>
#include <x86intrin.h>

namespace aoc_2022_20 {

constexpr int index_shift = 16;

static size_t find_avx(const int64_t *p, const size_t n, const int64_t needle)
{
    const auto vneedle = _mm256_set1_epi64x(needle);
    size_t k = 0;

    // 4-way unrolled loop, comparing 16 elements at time.
    for (; k + 15 < n; k += 16) {
        const auto v0 = _mm256_lddqu_si256((const __m256i *)&p[k + 0]);
        const auto v1 = _mm256_lddqu_si256((const __m256i *)&p[k + 4]);
        const auto v2 = _mm256_lddqu_si256((const __m256i *)&p[k + 8]);
        const auto v3 = _mm256_lddqu_si256((const __m256i *)&p[k + 12]);

        const auto eq0 = _mm256_cmpeq_epi64(v0, vneedle);
        const auto eq1 = _mm256_cmpeq_epi64(v1, vneedle);
        const auto eq2 = _mm256_cmpeq_epi64(v2, vneedle);
        const auto eq3 = _mm256_cmpeq_epi64(v3, vneedle);

        const auto eq0ps = _mm256_castsi256_ps(eq0);
        const auto eq1ps = _mm256_castsi256_ps(eq1);
        const auto eq2ps = _mm256_castsi256_ps(eq2);
        const auto eq3ps = _mm256_castsi256_ps(eq3);

        const uint8_t masks01_32[] = {
            static_cast<uint8_t>(_mm256_movemask_ps(eq0ps)),
            static_cast<uint8_t>(_mm256_movemask_ps(eq1ps)),
            static_cast<uint8_t>(_mm256_movemask_ps(eq2ps)),
            static_cast<uint8_t>(_mm256_movemask_ps(eq3ps)),
        };

        int32_t masks01;
        memcpy(&masks01, masks01_32, sizeof(masks01));

        if (masks01)
            return k + __builtin_ctz(masks01) / 2;
    }

    // Vectorized loop for 4-15 remaining elements.
    for (; k + 4 < n; k += 4) {
        const auto v = _mm256_lddqu_si256((const __m256i *)&p[k]);
        const auto eq = _mm256_cmpeq_epi64(v, vneedle);

        if (const auto mask = _mm256_movemask_epi8(eq))
            return k + __builtin_ctz(mask) / 8;
    }

    // Scalar loop for 1-3 remaining elements.
    for (; k < n; k++)
        if (p[k] == needle)
            return k;

    __builtin_unreachable();
}

static void mix(const std::vector<int64_t> &original, std::vector<int64_t> &nums)
{
    int64_t *data = nums.data();
    const size_t size = nums.size();

    // Who needs linked lists when you have an L1 cache and a prefetcher?
    for (const auto x : original) {
        const size_t i = find_avx(data, size, x);
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

    const auto zero_index = find_avx(nums.data(), nums.size(), zero);
    return (nums[(zero_index + 1000) % nums.size()] >> index_shift) +
           (nums[(zero_index + 2000) % nums.size()] >> index_shift) +
           (nums[(zero_index + 3000) % nums.size()] >> index_shift);
}

void run(FILE *f)
{
    std::vector<int64_t> nums;

    int64_t zero = 0;
    int64_t n;
    for (int i = 0; fscanf(f, "%" PRId64 "\n", &n) == 1; i++) {
        // To disambiguate duplicates, pack the index into the lower 16 bits
        // and the original number into the high 48 bits. This way we don't
        // need to keep track of a separate index array.
        if (n == 0)
            zero = i;
        nums.push_back(n << index_shift | i);
    }

    fmt::print("{}\n", solve(nums, 1, 1, zero));
    fmt::print("{}\n", solve(nums, 811589153, 10, zero));
}

}
