#include "common.h"
#include "dense_set.h"

namespace aoc_2025_2 {

static small_vector<size_t, 16> proper_divisors(size_t n)
{
    small_vector<size_t, 16> result;
    for (size_t k = 1; k < n; ++k) {
        if (n % k == 0)
            result.push_back(k);
    }
    return result;
}

constexpr size_t make_repeating(const size_t n, const size_t times)
{
    size_t result = 0;
    for (size_t i = 0; i < times; ++i) {
        result = result * pow10i[digit_count_base10(n)] + n;
    }
    return result;
}
static_assert(make_repeating(123, 3) == 123123123);

static size_t count_reps_2(size_t a, size_t b)
{
    const size_t na = digit_count_base10(a);
    const size_t nb = digit_count_base10(b);

    dense_set<size_t> q;
    q.reserve(16);

    for (size_t n = na; n <= nb; ++n) {
        if (n % 2 != 0)
            continue;

        const size_t d = n / 2;

        for (size_t k = pow10i[d - 1]; k < pow10i[d]; ++k) {
            const size_t rep = make_repeating(k, n / d);
            if (a <= rep && rep <= b)
                q.insert(rep);
        }
    }

    return std::ranges::fold_left(q, 0, λab(a + b));
}

static size_t count_reps_any(size_t a, size_t b)
{
    const size_t na = digit_count_base10(a);
    const size_t nb = digit_count_base10(b);

    dense_set<size_t> q;
    q.reserve(16);

    for (size_t n = na; n <= nb; ++n) {
        for (const size_t d : proper_divisors(n)) {
            for (size_t k = pow10i[d - 1]; k < pow10i[d]; ++k) {
                const size_t rep = make_repeating(k, n / d);
                if (a <= rep && rep <= b) {
                    // Use a set here to avoid double-counting (e.g. 11111
                    // counts for d=1, d=2 and d=5).
                    q.insert(rep);
                }
            }
        }
    }

    return std::ranges::fold_left(q, 0, λab(a + b));
}

void run(std::string_view buf)
{
    std::vector<std::string_view> ranges;
    split(buf, ranges, ',');

    size_t part1 = 0;
    size_t part2 = 0;

    for (auto r : ranges) {
        auto [a, b] = find_numbers_n<size_t, 2>(r);

        part1 += count_reps_2(a, b);
        part2 += count_reps_any(a, b);
    }

    fmt::print("{}\n{}\n", part1, part2);
}

}
