#include "common.h"

namespace aoc_2016_15 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::vector<int> moduli;
    moduli.reserve(lines.size() + 1);
    std::vector<int> rhs;
    rhs.reserve(lines.size() + 1);

    for (int i = 0; std::string_view line : lines) {
        auto [_1, m, _2, pos] = find_numbers_n<int, 4>(line);
        moduli.push_back(m);
        rhs.push_back(modulo<int>(-pos - i - 1, m));
        i++;
    }

    auto solve = [&rhs, &moduli] {
        // Chinese remainder theorem:
        int64_t M = 1;
        for (int64_t m : moduli)
            M *= m;

        int64_t x = 0;
        for (size_t i = 0; i < moduli.size(); ++i) {
            int64_t a = rhs[i];
            int64_t b = M / moduli[i];
            x += a * b * modinv(b, moduli[i]);
        }

        return x % M;
    };

    fmt::print("{}\n", solve());
    rhs.push_back(modulo<int>(10 - lines.size(), 11));
    moduli.push_back(11);
    fmt::print("{}\n", solve());
}

}
