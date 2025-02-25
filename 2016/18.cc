#include "common.h"

namespace aoc_2016_18 {

void run(std::string_view buf)
{
    std::array<uint64_t, 2> cells{};
    for (size_t i = 0; i < buf.size(); ++i)
        cells[i / 64] |= (buf[i] == '^') ? uint64_t(1) << (i % 64) : 0;

    const auto mask = (uint64_t(1) << (buf.size() - 64)) - 1;

    int traps = 0;
    int i = 0;
    auto f = [&](int bound) {
        for (; i < bound; i++) {
            traps += std::popcount(cells[1]) + std::popcount(cells[0]);
            std::array<uint64_t, 2> l = cells, r = cells;

            // Left and right-shift of {cells[1], cells[0]} interpreted as a
            // 128-bit integer. This sequence is about ~10% faster on my setup
            // compared to just using __uint128_t directly, despite all of the
            // implicit dependencies on the flags register.
            asm("shlq %0; rclq %1; shrq %2; rcrq %3\n"
                : "+r"(l[0]), "+r"(l[1]), "+r"(r[1]), "+r"(r[0])
                :
                : "cc");

            cells[0] = l[0] ^ r[0];
            cells[1] = (l[1] ^ r[1]) & mask;
        }
        return buf.size() * bound - traps;
    };

    const auto s1 = f(40);
    const auto s2 = f(400'000);
    fmt::print("{}\n{}\n", s1, s2);
}

}
