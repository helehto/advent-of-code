#include <algorithm>
#include <aoc/base.h>
#include <aoc/macros.h>
#include <aoc/math.h>
#include <aoc/string.h>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace aoc_2018_13 {

enum { N, E, S, W };

static Vec2u8 step(Vec2u8 p, uint8_t d)
{
    uint8_t dx = d == W ? -1 : d == E ? 1 : 0;
    uint8_t dy = d == N ? -1 : d == S ? 1 : 0;
    return p + Vec2u8(dx, dy);
}

struct Cart {
    Vec2u8 p;
    uint16_t direction : 2;
    uint16_t crossings : 14 = 0;
};

void run(std::string_view buf, aoc::Answer &answer)
{
    auto lines = split_lines(buf);
    Matrix<char> grid(lines.size(), lines[0].size(), ' ');
    for (size_t i = 0; i < lines.size(); i++)
        std::ranges::copy(lines[i], grid.row(i).begin());

    std::vector<Cart> carts;
    for (size_t i = 0; i < grid.rows; ++i) {
        for (size_t j = 0; j < grid.cols; ++j) {
            uint8_t d;
            char &c = grid(i, j);
            if (c == '<')
                d = W;
            else if (c == '>')
                d = E;
            else if (c == '^')
                d = N;
            else if (c == 'v')
                d = S;
            else
                continue;
            c = "|-"[d & 1];
            carts.emplace_back(Vec2u8(j, i), d);
        }
    }

    bool first_printed = false;
    while (carts.size() > 1) {
        std::ranges::sort(carts, {}, λa(std::pair(a.p.y, a.p.x)));

        size_t i = 0;
    restart:
        while (i < carts.size()) {
            auto &c = carts[i];

            c.p = step(c.p, c.direction);
            if (grid(c.p) == '/') {
                c.direction ^= 0b01;
            } else if (grid(c.p) == '\\') {
                c.direction ^= 0b11;
            } else if (grid(c.p) == '+') {
                if (c.crossings % 3 != 1)
                    c.direction = (c.direction + c.crossings % 3 + 3) & 3;
                c.crossings++;
            }

            size_t j = 0;
            for (; j < carts.size(); j++) {
                auto &c2 = carts[j];
                if (&c != &c2 && c.p == c2.p) {
                    if (!first_printed) {
                        answer.add_formatted("{},{}", c.p.x, c.p.y);
                        first_printed = true;
                    }

                    carts.erase(carts.begin() + std::max(i, j));
                    carts.erase(carts.begin() + std::min(i, j));
                    i -= j < i;
                    goto restart;
                }
            }

            i++;
        }
    }

    answer.add_formatted("{},{}", carts[0].p.x, carts[0].p.y);
}
AOC_REGISTER_SOLVER(2018, 13, run);

}
