#include <aoc/base.h>
#include <aoc/string.h>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace aoc_2020_6 {

void run(std::string_view buf, aoc::Answer &answer)
{
    auto lines = split_lines(buf);

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

    answer.add(any_count);
    answer.add(all_count);
}
AOC_REGISTER_SOLVER(2020, 6, run);

}
