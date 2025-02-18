#include "common.h"
#include <vector>

namespace aoc_2021_1 {

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

void run(std::string_view buf)
{
    std::vector<int> xs;

    for (std::string_view s : split_lines(buf)) {
        int x = 0;
        std::from_chars(s.data(), s.data() + s.size(), x);
        xs.push_back(x);
    }

    fmt::print("{}\n", solve(xs, 1));
    fmt::print("{}\n", solve(xs, 3));
}

}
