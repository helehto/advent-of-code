#include "common.h"
#include "dense_set.h"

namespace aoc_2022_9 {

static Vec2i move_knot(const Vec2i &dst, Vec2i src)
{
    const Vec2i d = dst - src;

    if (abs(d.x) > 1 || abs(d.y) > 1) {
        src.x += signum(d.x);
        src.y += signum(d.y);
    }

    return src;
}

static void move(std::span<Vec2i> pos, dense_set<Vec2i> &tail_positions, Vec2i d, int n)
{
    for (int i = 0; i < n; i++) {
        // Move the head.
        pos[0] += d;

        // Update the tail.
        for (size_t i = 1; i < pos.size(); i++)
            pos[i] = move_knot(pos[i - 1], pos[i]);

        tail_positions.insert(pos[pos.size() - 1]);
    }
}

void run(std::string_view buf)
{
    std::array<Vec2i, 2> s1{};
    std::array<Vec2i, 10> s2{};
    dense_set<Vec2i> p1;
    dense_set<Vec2i> p2;

    for (std::string_view line : split_lines(buf)) {
        const char c = line[0];
        int n = 0;
        std::from_chars(line.begin() + 2, line.end(), n);
        Vec2i d{};

        if (c == 'U')
            d.y = 1;
        else if (c == 'D')
            d.y = -1;
        else if (c == 'L')
            d.x = -1;
        else if (c == 'R')
            d.x = 1;

        move(s1, p1, d, n);
        move(s2, p2, d, n);
    }

    fmt::print("{}\n", p1.size());
    fmt::print("{}\n", p2.size());
}

}
