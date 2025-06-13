#include "common.h"
#include "dense_map.h"

namespace aoc_2020_25 {

/// Compute the discrete logarithm log_b(x) in GF(p) using Shanks' baby-step
/// giant-step algorithm.
static int discrete_log(int x, const int b, const int p)
{
    const int s = std::ceil(std::sqrt(p));

    dense_map<int, int> table;
    table.reserve(s);
    for (int j = 0, k = 1; j < s; ++j, k = (k * b) % p)
        table.emplace(k, j);

    auto inv = modinv(modexp(b, s, p), p);

    for (int i = 0; i < s; ++i, x = (x * inv) % p)
        if (auto it = table.find(x); it != table.end())
            return i * s + it->second;

    std::unreachable();
}

void run(std::string_view buf)
{
    auto [k1, k2] = find_numbers_n<int, 2>(buf);
    fmt::print("{}\n", modexp(k1, discrete_log(k2, 7, 20201227), 20201227));
}

}
