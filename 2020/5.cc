#include "common.h"

namespace aoc_2020_5 {

constexpr static auto char2bit = [] {
    std::array<uint8_t, 256> bit;
    bit.fill(1);
    bit['F'] = 0;
    bit['L'] = 0;
    return bit;
}();

void run(std::string_view buf)
{
    uint64_t id_sum = 0;
    int min_id = INT_MAX;
    int max_id = 0;
    for (std::string_view line : split_lines(buf)) {
        int id = 0;
        for (const uint8_t c : line)
            id = (id << 1) | char2bit[c];
        min_id = std::min(min_id, id);
        max_id = std::max(max_id, id);
        id_sum += id;
    }

    fmt::print("{}\n", max_id);
    fmt::print("{}\n", max_id * (max_id + 1) / 2 - min_id * (min_id - 1) / 2 - id_sum);
}

}
