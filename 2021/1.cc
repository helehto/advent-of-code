#include <aoc/base.h>
#include <aoc/small_vector.h>
#include <aoc/string.h>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

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

void run(std::string_view buf, aoc::Answer &answer)
{
    small_vector<int16_t, 2048> xs;
    find_numbers(buf, xs);
    answer.add(solve(xs, 1));
    answer.add(solve(xs, 3));
}
AOC_REGISTER_SOLVER(2021, 1, run);

}
