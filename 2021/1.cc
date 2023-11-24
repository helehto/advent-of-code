#include "common.h"
#include <fmt/core.h>
#include <span>
#include <vector>

static int solve(std::span<int> input, size_t window_size)
{
    int count = 0;
    int sum = 0;

    for (size_t i = 0; i < window_size; ++i)
        sum += input[i];

    for (size_t i = window_size; i < input.size(); ++i) {
        int prevsum = sum;
        sum += input[i] - input[i - window_size];
        if (sum > prevsum)
            count++;
    }

    return count;
}

void run_2021_1(FILE *f)
{
    std::string s;
    std::vector<int> xs;

    while (getline(f, s)) {
        int x = 0;
        std::from_chars(s.data(), s.data() + s.size(), x);
        xs.push_back(x);
    }

    fmt::print("{}\n", solve(xs, 1));
    fmt::print("{}\n", solve(xs, 3));
}
