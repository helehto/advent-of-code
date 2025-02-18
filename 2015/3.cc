#include "common.h"
#include "dense_set.h"

namespace aoc_2015_3 {

void run(std::string_view buf)
{
    {
        dense_set<Vec2i> points;
        points.insert({0, 0});

        Vec2i p = {0, 0};
        for (auto c : buf) {
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
        dense_set<Vec2i> points;
        points.insert({0, 0});

        Vec2i ps[2] = {};
        size_t i = 0;
        for (auto c : buf) {
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
