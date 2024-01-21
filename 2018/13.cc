#include "common.h"

namespace aoc_2018_13 {

enum { N, E, S, W };

static Point<uint8_t> step(Point<uint8_t> p, uint8_t d)
{
    int dx = d == W ? -1 : d == E ? 1 : 0;
    int dy = d == N ? -1 : d == S ? 1 : 0;
    return p.translate(dx, dy);
}

struct Cart {
    Point<uint8_t> p;
    uint16_t direction : 2;
    uint16_t crossings : 14 = 0;
};

void run(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);

    auto grid = Matrix<char>::from_lines(lines);

    std::vector<Cart> carts;
    for (auto p : grid.ndindex<uint8_t>()) {
        uint8_t d;
        char &c = grid(p);
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
        carts.emplace_back(p, d);
    }

    bool first_printed = false;
    while (carts.size() > 1) {
        std::ranges::sort(carts, {}, [](auto &c) { return std::pair(c.p.y, c.p.x); });

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
                        fmt::print("{},{}\n", c.p.x, c.p.y);
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

    fmt::print("{},{}\n", carts[0].p.x, carts[0].p.y);
}

}
