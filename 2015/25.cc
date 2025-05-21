#include "common.h"

namespace aoc_2015_25 {

/// Compute a^b mod m.
constexpr int64_t modexp(int64_t a, int64_t b, const int64_t m)
{
    int64_t result = 1;
    for (; b; b >>= 1) {
        if (b & 1)
            result = (a * result) % m;
        a = (a * a) % m;
    }
    return result;
}

void run(std::string_view buf)
{
    const auto [row, col] = find_numbers_n<int, 2>(buf);
    const int diag = row + col - 1;
    const int k = diag * (diag + 1) / 2 - row + 1;
    fmt::print("{}\n", (20151125 * modexp(252533, k - 1, 33554393)) % 33554393);
}

}
