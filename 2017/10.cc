#include "knot_hash.h"
#include <aoc/base.h>
#include <aoc/string.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace aoc_2017_10 {

static int part1(std::string_view buf)
{
    auto lengths = find_numbers<uint8_t>(buf);
    KnotHash k;
    for (const size_t n : lengths)
        k.sparse_round(n);
    return k.ring[0] * k.ring[1];
}

static std::array<char, 33> part2(std::string_view buf)
{
    return as_hex(KnotHash{}.as_bytes(
        std::span(reinterpret_cast<const uint8_t *>(buf.data()), buf.size())));
}

void run(std::string_view buf, aoc::Answer &answer)
{
    answer.add(part1(buf));
    answer.add(std::string_view(part2(buf).data()));
}
AOC_REGISTER_SOLVER(2017, 10, run);

}
