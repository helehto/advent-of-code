#include "common.h"
#include "dense_map.h"

namespace aoc_2017_13 {

struct Scanner {
    uint8_t depth;
    uint8_t range;
};

constexpr int part1(std::span<const Scanner> scanners)
{
    int result = 0;
    for (const auto [depth, range] : scanners)
        if (depth % (2 * range - 2) == 0)
            result += depth * range;
    return result;
}

static int part2(std::span<const Scanner> scanners)
{
    dense_map<int, small_vector<uint8_t>> forbidden_remainders;
    forbidden_remainders.reserve(16);

    uint64_t candidate_mask_mod64 = UINT64_MAX;

    for (const auto [depth, range] : scanners) {
        int divisor = 2 * range - 2;
        int remainder = depth ? (-depth) % divisor + divisor : 0;
        remainder = (remainder != divisor) ? remainder : 0;

        if (divisor < 64 && (divisor & (divisor - 1)) == 0) {
            // Treat small powers of two less than 64 specially -- these can be
            // combined into a single 64-bit mask which allows for cheaply
            // iterating through the set bits and skipping invalid solutions
            // with blsr+tzcount.
            uint64_t prohibited_mask = (UINT64_MAX / ((UINT64_C(1) << divisor) - 1))
                                       << remainder;
            candidate_mask_mod64 &= ~prohibited_mask;
        } else {
            auto &fr = forbidden_remainders[divisor];
            if (!std::ranges::contains(fr, remainder))
                fr.push_back(remainder);
        }
    }

    struct SieveEntry {
        /// Vector containing 8 copies of the divisor.
        __m256 d;

        /// The set of remainders which are forbidden; in other words, if the
        /// remainder of dividing the duration we wait by `d` matches _any_ of
        /// these, we get caught.
        __m256 forbidden_remainders;

        SieveEntry(size_t divisor, const std::span<const uint8_t> forbidden)
            : d(_mm256_set1_ps(static_cast<float>(divisor)))
        {
            DEBUG_ASSERT(forbidden.size() <= 8);
            DEBUG_ASSERT(divisor < (1 << 23));

            // -1.0f acts as a dummy value if the set of forbidden remainders
            // for this entry does not fill an entire AVX vector of 8 floats --
            // since nothing will have a negative remainder, these values will
            // not filter out any valid solution.
            std::array<float, 8> fr{-1.0f, -1.0f, -1.0f, -1.0f,
                                    -1.0f, -1.0f, -1.0f, -1.0f};
            std::ranges::copy(forbidden, fr.begin());
            forbidden_remainders = _mm256_load_ps((const float *)fr.data());
        }

        bool caught(const __m256 t) const noexcept
        {
            // Compute the remainder t % d with floating-point math:
            const __m256 q = _mm256_div_ps(t, d);       // t/d
            const __m256 f = _mm256_floor_ps(q);        // ⌊t/d⌋
            const __m256 r = _mm256_fnmadd_ps(f, d, t); // t - ⌊t/d⌋ * d

            // Does the computed remainder match any forbidden remainder?
            const __m256 c = _mm256_cmp_ps(r, forbidden_remainders, _CMP_EQ_OQ);
            return !_mm256_testz_ps(c, c);
        }

        bool caught(const float t) const noexcept
        {
            DEBUG_ASSERT(t < (1 << 23));
            return caught(_mm256_set1_ps(t));
        }
    };

    // Construct the sieve entries, splitting divisors with more than 8
    // forbidden remainders across multiple entries.
    small_vector<SieveEntry, 16> sieve_entries;
    for (auto &[i, fr] : forbidden_remainders) {
        for (std::span f = fr; !f.empty();) {
            const auto n = std::min<size_t>(f.size(), 8);
            sieve_entries.emplace_back(i, f.first(n));
            f = f.subspan(n);
        }
    }

    for (int tt = 0;; tt += 64) {
        for (uint64_t m = candidate_mask_mod64; m; m &= m - 1) {
            int t = tt + std::countr_zero(m);
            if (std::ranges::none_of(sieve_entries, λ(e, e.caught(t))))
                return t;
        }
    }
}

void run(std::string_view buf)
{
    small_vector<uint8_t, 256> nums;
    find_numbers(buf, nums);
    std::span scanners(reinterpret_cast<Scanner *>(nums.data()), nums.size() / 2);
    fmt::print("{}\n", part1(scanners));
    fmt::print("{}\n", part2(scanners));
}

}
