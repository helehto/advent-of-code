#include "common.h"

namespace aoc_2025_1 {

// Pre-computed table for computing modulo 100 for small inputs.
constexpr auto modulo_table = [] consteval {
    std::array<int8_t, 4000> result;
    for (size_t i = 0; i < result.size() / 100; ++i)
        for (size_t j = 0; j < 100; ++j)
            result[100 * i + j] = j;
    return result;
}();

void run(std::string_view buf)
{
    int k = 50;
    int part1 = 0;
    int part2 = 0;

    const char *p = buf.data();
    const char *q = p + buf.size();

    constexpr const int8_t *mod100 = &modulo_table[modulo_table.size() / 2];
    while (p < q) {
        // -1 for L, 0 for R.
        const int32_t dir = (static_cast<int32_t>(*p) - 'L' - 1) >> 31;
        p++;

        // Parse number manually, since we know it's at most 3 digits long:
        int n = 0;
        for (size_t i = 0; i < 3 && *p != '\n' && p < q; ++i)
            n = 10 * n + *p++ - '0';
        p++;

        const int oldk = k;
        k = dir < 0 ? mod100[k - n] : mod100[k + n];
        part1 += k == 0;

        // TODO: Bleh. Clean this mess up.
        part2 += n / 100;
        if (oldk != 0) [[likely]] {
            part2 += (dir < 0 && (k > oldk || k == 0));
            part2 += (dir == 0 && (k < oldk || k == 0));
        }
    }

    fmt::print("{}\n{}\n", part1, part2);
}

}
