#include "common.h"
#include <numeric>

namespace aoc_2023_8 {

constexpr uint16_t encode_name(std::string_view sv)
{
    return (sv[0] - 'A') << 10 | (sv[1] - 'A') << 5 | (sv[2] - 'A');
}

void run(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);
    std::string_view directions = lines[0];

    std::vector<std::array<uint16_t, 2>> next(1 << 15);
    std::vector<size_t> starts;
    for (size_t i = 2; i < lines.size(); i++) {
        std::string_view sv = lines[i];
        const uint16_t ai = encode_name(sv.substr(0, 3));
        const uint16_t bi = encode_name(sv.substr(7, 3));
        const uint16_t ci = encode_name(sv.substr(12, 3));
        next[ai] = {{bi, ci}};
        if ((ai & 0b11111) == 0)
            starts.push_back(ai);
    }

    auto walk = [&](size_t start) {
        size_t n = 0;
        for (size_t i = 0, k = start; (k & 0b11111) != 25; i++, n++) {
            if (i >= directions.size())
                i = 0;
            k = next[k][directions[i] == 'R'];
        }
        return n;
    };

    fmt::print("{}\n", walk(encode_name("AAA")));

    uint64_t part2 = 1;
    for (size_t k : starts)
        part2 = std::lcm(part2, walk(k));
    fmt::print("{}\n", part2);
}

}
