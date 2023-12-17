#include "common.h"
#include "dense_set.h"
#include "dense_map.h"

enum { N, E, S, W };

static Point<uint8_t> step(Point<uint8_t> p, int d)
{
    static int8_t table[] = {0, 1, 0, -1};
    return p.translate(table[d], table[(d - 1) & 3]);
}

static uint16_t encode_state(Point<uint8_t> p, int dir)
{
    return p.x | static_cast<uint16_t>(p.y) << 7 | static_cast<uint16_t>(dir) << 14;
}

static const dense_set<Point<uint8_t>> &
fire(dense_map<uint16_t, dense_set<Point<uint8_t>>> &cache,
     const Matrix<char> &grid,
     Point<uint8_t> p,
     int dir)
{
    auto [it, inserted] = cache.try_emplace(encode_state(p, dir));
    if (!inserted)
        return it->second;
    auto &visited = it->second;

    p = step(p, dir);
    for (; grid.in_bounds(p); p = step(p, dir)) {
        visited.insert(p);

        if (auto it = cache.find(encode_state(p, dir)); it != cache.end()) {
            visited.insert(it->second.begin(), it->second.end());
            break;
        }

        if (grid(p) == "-|"[dir & 1]) {
            auto &c1 = fire(cache, grid, p, (dir + 1) & 3);
            auto &c2 = fire(cache, grid, p, (dir - 1) & 3);
            visited.reserve(visited.size() + c1.size() + c2.size());
            visited.insert(c1.begin(), c1.end());
            visited.insert(c2.begin(), c2.end());
            break;
        } else if (grid(p) == '/') {
            dir ^= 0b01;
        } else if (grid(p) == '\\') {
            dir ^= 0b11;
        }
    }

    return visited;
}

void run_2023_16(FILE *f)
{
    auto grid = Matrix<char>::from_lines(getlines(f));
    dense_map<uint16_t, dense_set<Point<uint8_t>>> cache(grid.size());

    size_t part2 = 0;
    auto fire_from_edge = [&](uint8_t x, uint8_t y, int dir) {
        Point<uint8_t> p = step({x, y}, (dir + 2) & 3);
        return fire(cache, grid, p, dir).size();
    };
    fmt::print("{}\n", fire_from_edge(0, 0, E));

    for (uint8_t x = 0; x < grid.cols; x++) {
        part2 = std::max(part2, fire_from_edge(x, 0, S));
        part2 = std::max(part2, fire_from_edge(x, grid.rows - 1, N));
    }
    for (uint8_t y = 0; y < grid.rows; y++) {
        part2 = std::max(part2, fire_from_edge(0, y, E));
        part2 = std::max(part2, fire_from_edge(grid.cols - 1, y, W));
    }
    fmt::print("{}\n", part2);
}
