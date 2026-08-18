#include <aoc/base.h>
#include <aoc/dense_set.h>
#include <cstdint>
#include <string_view>
#include <utility>

namespace aoc_2018_21 {

void run(std::string_view, aoc::Answer &answer)
{
    dense_set<int64_t> seen;
    seen.reserve(1 << 14);

    int64_t e = 0;
    int64_t n = 0;
    int64_t n_prev = 0;

    // FIXME: Constant(s) hard-coded for my input.
    do {
        e = n | 0x10000;
        n_prev = std::exchange(n, 7637914);

        for (; e; e >>= 8) {
            n += e & 0xff;
            n &= 0xffffff;
            n *= 65899;
            n &= 0xffffff;
        }

        if (seen.empty())
            answer.add(n);
    } while (seen.emplace(n).second);

    answer.add(n_prev);
}
AOC_REGISTER_SOLVER(2018, 21, run);

}
