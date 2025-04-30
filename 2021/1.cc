#include "common.h"

namespace aoc_2021_1 {

static int solve(std::span<const int16_t> input, size_t window_size)
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

void run(std::string_view buf)
{
    small_vector<int16_t, 2048> xs;
    find_numbers(buf, xs);
    fmt::print("{}\n", solve(xs, 1));
    fmt::print("{}\n", solve(xs, 3));
}
}
