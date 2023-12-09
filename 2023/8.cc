#include "common.h"
#include <numeric>

constexpr uint16_t encode_name(std::string_view sv)
{
    return (sv[0] - 'A') << 10 | (sv[1] - 'A') << 5 | (sv[2] - 'A');
}

void run_2023_8(FILE *f)
{
    std::string directions;
    getline(f, directions);
    std::string s;
    getline(f, s);

    std::vector<std::array<uint16_t, 2>> next(1 << 15);
    std::vector<size_t> starts;
    while (getline(f, s)) {
        std::string_view sv = s;
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
