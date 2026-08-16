#include "common.h"

namespace aoc_2016_19 {

void run(std::string_view buf, aoc::Answer &answer)
{
    auto [n] = find_numbers_n<uint32_t, 1>(buf);

    // Part 1 is a special case of the Josephus problem with k=2:
    // https://en.wikipedia.org/wiki/Josephus_problem
    answer.add(((n << 1) & ~std::bit_ceil(n)) | 1);

    // Part 2:
    uint32_t i = 1;
    while (i * 3 < n)
        i *= 3;
    answer.add(n - i);
}
AOC_REGISTER_SOLVER(2016, 19, run);

}
