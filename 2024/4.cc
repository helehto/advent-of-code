#include "common.h"

namespace aoc_2024_4 {

constexpr bool scan(MatrixView<const char> grid, Vec2i start, Vec2i d, int len, char *out)
{
    for (int n = 0; n < len; ++n) {
        auto p = start + n * d;
        if (!grid.in_bounds(p))
            return false;
        *out++ = grid(p);
    }
    return true;
}

constexpr bool
match_word(const std::array<char, 4> &word, char c1, char c2, char c3, char c4)
{
    // Compare words using a single 32-bit comparison.
    return std::bit_cast<uint32_t>(word) ==
           std::bit_cast<uint32_t>(std::array<char, 4>{c1, c2, c3, c4});
}

constexpr int part1(MatrixView<const char> grid)
{
    alignas(4) std::array<char, 4> word;
    int result = 0;

    for (auto p : grid.ndindex<int>())
        for (int dx : {-1, 0, 1})
            for (int dy : {-1, 0, 1})
                result += scan(grid, p, {dx, dy}, 4, word.data()) &&
                          match_word(word, 'X', 'M', 'A', 'S');

    return result;
}

constexpr int part2(MatrixView<const char> grid)
{
    alignas(4) std::array<char, 4> word;
    int result = 0;

    for (auto p : grid.ndindex<int>()) {
        auto check = [&](Vec2i start, int dx, int dy) {
            word.fill(0);
            return scan(grid, start, {dx, dy}, 3, word.data()) &&
                   (match_word(word, 'M', 'A', 'S', 0) ||
                    match_word(word, 'S', 'A', 'M', 0));
        };

        result += check(p, 1, 1) && check(p + Vec2i(2, 0), -1, 1);
    }

    return result;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines);
    fmt::print("{}\n", part1(grid));
    fmt::print("{}\n", part2(grid));
}

}
