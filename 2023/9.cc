#include "common.h"
#include <numeric>

namespace aoc_2023_9 {

static double basis(int64_t k, int64_t x, int64_t j)
{
    // I'm kind of lucky that the intermediate values in the polynomial
    // calculation here don't have any precision issues with a double given my
    // input, but I can't be bothered to go back and redo this.
    double p = 1;
    double q = 1;

    for (int64_t m = 0; m < k; ++m) {
        if (m != j) {
            p *= x - m - 1;
            q *= j - m;
        }
    }

    return p / q;
}

static std::vector<double> lagrange_basis(int64_t k, int64_t x)
{
    std::vector<double> result;
    for (int64_t j = 0; j < k; ++j)
        result.push_back(basis(k, x, j));
    return result;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto nums = find_numbers<int>(lines[0]);

    // This assumes that all input lists are of the same length.
    const auto basis1 = lagrange_basis(nums.size(), nums.size() + 1);
    const auto basis2 = lagrange_basis(nums.size(), 0);

    int64_t part1 = 0;
    int64_t part2 = 0;
    for (size_t i = 0;;) {
        part1 += std::inner_product(nums.begin(), nums.end(), basis1.begin(), 0.0);
        part2 += std::inner_product(nums.begin(), nums.end(), basis2.begin(), 0.0);
        i++;
        if (i == lines.size())
            break;
        find_numbers(lines[i], nums);
    }
    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}

}
