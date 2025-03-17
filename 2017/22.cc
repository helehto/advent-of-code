#include "common.h"
#include "dense_map.h"
#include "dense_set.h"

namespace aoc_2017_22 {

enum { CLEAN = '.', WEAKENED = 'W', INFECTED = '#', FLAGGED = 'F' };

static std::pair<dense_set<Vec2i>, Vec2i> parse_input(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines);

    dense_set<Vec2i> cells;
    cells.reserve(grid.rows * grid.cols);
    for (auto p : grid.ndindex<int>())
        if (grid(p) == INFECTED)
            cells.insert(p);

    return {cells, Vec2i(grid.rows / 2, grid.cols / 2)};
}

static int part1(dense_set<Vec2i> cells, const Vec2i start)
{
    Vec2i p = start;
    Vec2i d(0, -1);

    int infections = 0;
    for (int i = 0; i < 10'000; ++i) {
        auto it = cells.find(p);
        if (it != cells.end()) {
            d = d.cw();
            cells.erase(p);
        } else {
            d = d.ccw();
            cells.insert(p);
            infections++;
        }
        p += d;
    }

    return infections;
}

static int part2(dense_set<Vec2i> cellsx, const Vec2i start)
{
    Vec2i p = start;
    Vec2i d(0, -1);

    dense_map<Vec2i, char> cells;
    cells.reserve(cellsx.size());
    for (auto &p : cellsx)
        cells[p] = INFECTED;

    int infections = 0;
    for (int i = 0; i < 10'000'000; ++i) {
        auto &state = cells.emplace(p, CLEAN).first->second;

        if (state == CLEAN)
            d = d.ccw();
        else if (state == INFECTED)
            d = d.cw();
        else if (state == FLAGGED)
            d = -d;

        if (state == CLEAN) {
            state = WEAKENED;
        } else if (state == WEAKENED) {
            state = INFECTED;
            infections++;
        } else if (state == INFECTED) {
            state = FLAGGED;
        } else {
            state = CLEAN;
        }

        p += d;
    }

    return infections;
}

void run(std::string_view buf)
{
    auto [cells, start] = parse_input(buf);
    fmt::print("{}\n", part1(cells, start));
    fmt::print("{}\n", part2(cells, start));
}

}
