#include "common.h"
#include "dense_set.h"

namespace aoc_2015_3 {

void run(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);

    {
        dense_set<Point<int>> points;
        points.insert({0, 0});

        Point<int> p = {0, 0};
        for (auto c : lines[0]) {
            switch (c) {
            case '>':
                p.x++;
                break;
            case '<':
                p.x--;
                break;
            case 'v':
                p.y++;
                break;
            case '^':
                p.y--;
                break;
            }
            points.insert(p);
        }

        fmt::print("{}\n", points.size());
    }

    {
        dense_set<Point<int>> points;
        points.insert({0, 0});

        Point<int> ps[2] = {};
        size_t i = 0;
        for (auto c : lines[0]) {
            switch (c) {
            case '>':
                ps[i & 1].x++;
                break;
            case '<':
                ps[i & 1].x--;
                break;
            case 'v':
                ps[i & 1].y++;
                break;
            case '^':
                ps[i & 1].y--;
                break;
            }
            points.insert(ps[i & 1]);
            i++;
        }

        fmt::print("{}\n", points.size());
    }
}

}
