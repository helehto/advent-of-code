#include "common.h"
#include "dense_set.h"

namespace aoc_2022_9 {

static Point<int> move_knot(const Point<int> &dst, Point<int> src)
{
    const int dx = dst.x - src.x;
    const int dy = dst.y - src.y;

    if (abs(dy) > 1 || abs(dx) > 1) {
        src.x += signum(dx);
        src.y += signum(dy);
    }

    return src;
}

static void move(std::span<Point<int>> pos,
                 dense_set<Point<int>> &tail_positions,
                 int dx,
                 int dy,
                 int n)
{
    for (int i = 0; i < n; i++) {
        // Move the head.
        pos[0] = pos[0].translate(dx, dy);

        // Update the tail.
        for (size_t i = 1; i < pos.size(); i++)
            pos[i] = move_knot(pos[i - 1], pos[i]);

        tail_positions.insert(pos[pos.size() - 1]);
    }
}

void run(std::string_view buf)
{
    std::array<Point<int>, 2> s1{};
    std::array<Point<int>, 10> s2{};
    dense_set<Point<int>> p1;
    dense_set<Point<int>> p2;

    for (std::string_view line : split_lines(buf)) {
        const char c = line[0];
        int n = 0;
        std::from_chars(line.begin() + 2, line.end(), n);
        int dx = 0;
        int dy = 0;

        if (c == 'U')
            dy = 1;
        else if (c == 'D')
            dy = -1;
        else if (c == 'L')
            dx = -1;
        else if (c == 'R')
            dx = 1;

        move(s1, p1, dx, dy, n);
        move(s2, p2, dx, dy, n);
    }

    fmt::print("{}\n", p1.size());
    fmt::print("{}\n", p2.size());
}

}
