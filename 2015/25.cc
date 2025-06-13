#include "common.h"

namespace aoc_2015_25 {

void run(std::string_view buf)
{
    const auto [row, col] = find_numbers_n<int, 2>(buf);
    const int diag = row + col - 1;
    const int k = diag * (diag + 1) / 2 - row + 1;
    fmt::print("{}\n", (20151125 * modexp(252533, k - 1, 33554393)) % 33554393);
}

}
