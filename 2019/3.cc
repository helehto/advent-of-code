#include "common.h"
#include "dense_map.h"

namespace aoc_2019_3 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    dense_map<Vec2i16, int8_t> visited;
    dense_map<Vec2i16, int> steps_maps[2];

    std::vector<std::string_view> moves;
    for (size_t i = 0; i < 2; i++) {
        split(lines[i], moves, ',');

        Vec2i16 p{0, 0};
        int steps = 0;
        for (std::string_view w : moves) {
            int n = 0, dx = 0, dy = 0;
            std::from_chars(w.data() + 1, w.data() + w.size(), n);
            if (w[0] == 'U')
                dy = 1;
            if (w[0] == 'D')
                dy = -1;
            if (w[0] == 'L')
                dx = -1;
            if (w[0] == 'R')
                dx = 1;

            for (int j = 0; j < n; j++) {
                p = p.translate(dx, dy);
                visited[p] |= 1 << i;
                steps_maps[i].emplace(p, ++steps);
            }
        }
    };

    std::vector<Vec2i16> crossings;
    crossings.reserve(visited.size());
    for (auto &[p, count] : visited) {
        if (count == 3)
            crossings.push_back(p);
    }

    Vec2i16 minp = {1000, 1000};
    for (Vec2i16 &p : crossings) {
        if (manhattan(p) < manhattan(minp))
            minp = p;
    }

    int minsteps = INT_MAX;
    for (Vec2i16 &p : crossings) {
        int steps = steps_maps[0][p] + steps_maps[1][p];
        if (steps < minsteps)
            minsteps = steps;
    }

    fmt::print("{}\n{}\n", manhattan(minp), minsteps);
}

}
