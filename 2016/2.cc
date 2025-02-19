#include "common.h"

namespace aoc_2016_2 {

static int part1(const std::vector<std::string_view> &lines)
{
    int code = 0;

    Vec2i p{1, 1};
    for (std::string_view line : lines) {
        for (char c : line) {
            if (c == 'U')
                p.y = std::max(p.y - 1, 0);
            else if (c == 'L')
                p.x = std::max(p.x - 1, 0);
            else if (c == 'D')
                p.y = std::min(p.y + 1, 2);
            else if (c == 'R')
                p.x = std::min(p.x + 1, 2);
            else
                ASSERT(false);
        }

        code = 10 * code + 3 * p.y + p.x + 1;
    }

    return code;
}

static std::string part2(const std::vector<std::string_view> &lines)
{
    constexpr char keypad[] = "  1   234 56789 ABC   D  ";
    std::string code;
    code.reserve(lines.size());

    Vec2i p{-2, 0};
    for (std::string_view line : lines) {
        for (char c : line) {
            auto q = p;
            if (c == 'U')
                q.y--;
            else if (c == 'L')
                q.x--;
            else if (c == 'D')
                q.y++;
            else if (c == 'R')
                q.x++;
            else
                ASSERT(false);

            if (manhattan(q) <= 2)
                p = q;
        }

        code += keypad[5 * (p.y + 2) + p.x + 2];
    }

    return code;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    fmt::print("{}\n", part1(lines));
    fmt::print("{}\n", part2(lines));
}

}
