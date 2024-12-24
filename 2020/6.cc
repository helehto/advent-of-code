#include "common.h"
#include <bit>

namespace aoc_2020_6 {

void run(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);

    size_t i = 0;
    int any_count = 0;
    int all_count = 0;
    do {
        uint32_t any_mask = 0;
        uint32_t all_mask = (uint32_t(1) << ('z' - 'a' + 1)) - 1;
        for (; i < lines.size() && !lines[i].empty(); i++) {
            uint32_t person_mask = 0;
            for (const uint8_t c : lines[i])
                person_mask |= uint32_t(1) << (c - 'a');
            any_mask |= person_mask;
            all_mask &= person_mask;
        }
        any_count += std::popcount(any_mask);
        all_count += std::popcount(all_mask);
        i++;
    } while (i < lines.size());

    fmt::print("{}\n", any_count);
    fmt::print("{}\n", all_count);
}

}
