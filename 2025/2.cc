#include "common.h"
#include "dense_set.h"

namespace aoc_2025_2 {

/// To construct a repeating number with `n` repeated `k` times, we can simply
/// multiply `n` by an appropriate factor, e.g.:
///
///     7 * 111 = 777
///     23 * 10101010101 = 232323232323
///     12345 * 10000100001 = 123451234512345
///
/// This table contains precomputed factors for all relevant `n` and `k`.
constexpr auto repeater_factors = [] consteval {
    std::array<std::array<uint64_t, 18>, 18> result{};
    for (uint64_t i = 1; i < 18; ++i) {
        for (uint64_t j = 1; j < 18; ++j) {
            // Some of these entries will overflow uint64_t, but we don't care
            // since they won't be used.
            result[i][j] = result[i][j - 1] * pow10i[i] + 1;
        }
    }
    return result;
}();

static uint64_t sum_unique(small_vector_base<uint64_t> &r)
{
    std::ranges::sort(r);
    auto w = std::ranges::unique(r);
    r.erase(w.begin(), w.end());
    return std::ranges::fold_left(r, 0, λab(a + b));
}

static const inplace_vector<int8_t, 5> proper_divisors[] = {
    /* 0  */ {},
    /* 1  */ {},
    /* 2  */ {1},
    /* 3  */ {1},
    /* 4  */ {1, 2},
    /* 5  */ {1},
    /* 6  */ {1, 2, 3},
    /* 7  */ {1},
    /* 8  */ {1, 2, 4},
    /* 9  */ {1, 3},
    /* 10 */ {1, 2, 5},
    /* 11 */ {1},
    /* 12 */ {1, 2, 3, 4, 6},
    /* 13 */ {1},
    /* 14 */ {1, 2, 7},
    /* 15 */ {1, 3, 5},
    /* 16 */ {1, 2, 4, 8},
    /* 17 */ {1},
    /* 18 */ {1, 2, 3, 6, 9},
    /* 19 */ {1},
};

template <int Part>
static size_t count_reps_any(small_vector_base<uint64_t> &buffer, size_t a, size_t b)
{
    const size_t na = digit_count_base10(a);
    const size_t nb = digit_count_base10(b);

    buffer.clear();

    for (size_t n = na; n <= nb; ++n) {
        const uint64_t aa = std::clamp(a, pow10i[n - 1], pow10i[n]);
        const uint64_t bb = std::clamp(b, pow10i[n - 1], pow10i[n]);

        inplace_vector<int8_t, 5> divs;
        if constexpr (Part == 1) {
            if (n % 2 == 0)
                divs.push_back(n / 2);
        } else {
            divs = proper_divisors[n];
        }

        for (const int d : divs) {
            const uint64_t r = repeater_factors[d][n / d];
            const uint64_t lo = (aa + r - 1) / r;
            const uint64_t hi = (bb - 1) / r;

            // TODO: Use the inclusion-exclusion principle here to avoid
            // double-counting, instead of having to prune later...
            for (uint64_t k = lo; k <= hi; ++k)
                buffer.push_back(k * r);
        }
    }

    return sum_unique(buffer);
}

void run(std::string_view buf)
{
    std::vector<std::string_view> ranges;
    split(buf, ranges, ',');

    size_t part1 = 0;
    size_t part2 = 0;

    small_vector<uint64_t, 256> tmp;
    for (auto r : ranges) {
        auto [a, b] = find_numbers_n<size_t, 2>(r);
        part1 += count_reps_any<1>(tmp, a, b);
        part2 += count_reps_any<2>(tmp, a, b);
    }

    fmt::print("{}\n{}\n", part1, part2);
}

}
