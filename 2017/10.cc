#include "common.h"
#include "knot_hash.h"

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

void run(std::string_view buf)
{
    fmt::print("{}\n", part1(buf));
    fmt::print("{}\n", part2(buf).data());
}

}
