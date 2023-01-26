#include "common.h"
#include "dense_set.h"
#include <fmt/core.h>

void run_2015_3(FILE *f)
{
    std::string s;
    getline(f,s);

    {
        dense_set<Point<int>> points;
        points.insert({0, 0});

        Point<int> p = {0,0};
        for (auto c : s) {
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
        for (auto c : s) {
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
