#include "common.h"

namespace aoc_2016_19 {

void run(std::string_view buf)
{
    auto [n] = find_numbers_n<uint32_t, 1>(buf);

    // Part 1 is a special case of the Josephus problem with k=2:
    // https://en.wikipedia.org/wiki/Josephus_problem
    fmt::print("{}\n", ((n << 1) & ~std::bit_ceil(n)) | 1);

    // Part 2:
    uint32_t i = 1;
    while (i * 3 < n)
        i *= 3;
    fmt::print("{}\n", n - i);
}

}
