#include "common.h"
#include <fmt/ranges.h>
#include <span>

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

static double lagrange(std::span<const int> points, int64_t x)
{
    double result = 0;
    for (size_t j = 0; j < points.size(); ++j)
        result += points[j] * basis(points.size(), x, j);
    return result;
}

void run_2023_9(FILE *f)
{
    int64_t part1 = 0;
    int64_t part2 = 0;
    std::string s;
    std::vector<int> nums;
    while (getline(f, s)) {
        find_numbers(s, nums);
        part1 += lagrange(nums, nums.size() + 1);
        part2 += lagrange(nums, 0);
    }

    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}
