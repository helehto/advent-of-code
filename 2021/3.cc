#include "common.h"
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <vector>
#include <span>

static int most_common_bit(std::span<uint16_t> xs, size_t bit)
{
    uint32_t sum = 0;
    for (auto x : xs)
        sum += x & (1 << bit);
    sum >>= bit;
    return 2 * sum >= xs.size();
}

void run_2021_3(FILE *f)
{
    std::vector<uint16_t> xs;
    size_t bit_length = 0;

    std::string s;
    while (getline(f, s)) {
        uint32_t x = 0;
        for (char c : s)
            x = x << 1 | (c - '0');
        xs.push_back(x);
        bit_length = s.size();
    }

    uint16_t gamma = 0;
    uint16_t epsilon = 0;
    for (size_t bit = bit_length; bit--;) {
        int b = most_common_bit(xs, bit);
        gamma = gamma << 1 | b;
        epsilon = epsilon << 1 | (1 - b);
    }
    fmt::print("{}\n", gamma * epsilon);

    std::vector<uint16_t> candidates(xs);

    int oxygen_rating = 0;
    for (size_t bit = bit_length - 1; candidates.size() > 1; bit--) {
        int b = most_common_bit(candidates, bit);
        erase_if(candidates, [&](auto x) { return ((x >> bit) & 1) != b; });
    }
    oxygen_rating = candidates.front();

    int co2_rating = 0;
    candidates = xs;
    for (size_t bit = bit_length - 1; candidates.size() > 1; bit--) {
        int b = most_common_bit(candidates, bit);
        erase_if(candidates, [&](auto x) { return ((x >> bit) & 1) == b; });
    }
    co2_rating = candidates.front();

    fmt::print("{}\n", oxygen_rating * co2_rating);
}
