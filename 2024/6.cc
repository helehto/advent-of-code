#include "common.h"
#include "dense_set.h"

namespace aoc_2024_6 {

enum { N, E, S, W };
constexpr Vec2i dxdy[] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};

struct State {
    uint8_t x;
    uint8_t y;
    uint8_t dir;

    constexpr bool operator==(const State &) const = default;
};

static std::pair<std::vector<State>, size_t> walk1(MatrixView<const char> grid, Vec2u16 p)
{
    uint16_t dir = N;
    std::vector<State> visited{State(p.x, p.y, dir)};
    dense_set<Vec2u16> unique{p};

    while (true) {
        auto q = p + dxdy[dir];
        if (!grid.in_bounds(q))
            break;

        while (grid(q) == '#') {
            dir = (dir + 1) & 3;
            q = p + dxdy[dir];
        }

        visited.push_back(State(q.x, q.y, dir));
        unique.insert(q);
        p = q;
    }

    return std::pair(visited, unique.size());
}

static bool walk2(MatrixView<const char> grid,
                  Vec2u16 p,
                  std::array<Matrix<bool>, 4> &visited,
                  dense_set<Vec2u16> *out = nullptr)
{
    uint16_t dir = N;

    for (Matrix<bool> &m : visited)
        std::ranges::fill(m.all(), false);

    visited[dir](p) = true;

    while (true) {
        auto q = p + dxdy[dir];
        if (!grid.in_bounds(q))
            break;

        while (grid(q) == '#') {
            dir = (dir + 1) & 3;
            q = p + dxdy[dir];
        }

        if (visited[dir](q))
            return false;
        visited[dir](q) = true;

        p = q;
    }

    if (out) {
        out->clear();
        out->reserve(visited.size());
        for (size_t dir = 0; dir < 4; ++dir) {
            for (auto p : visited[dir].ndindex<uint16_t>()) {
                if (visited[dir](p))
                    out->insert(p);
            }
        }
    }

    return true;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines);
    ASSERT(grid.rows < 256);
    ASSERT(grid.cols < 256);

    Vec2u16 start{};
    for (auto p : grid.ndindex<uint16_t>()) {
        if (grid(p) == '^') {
            start = p;
            break;
        }
    }

    auto [route, n_route_unique] = walk1(grid, start);
    fmt::print("{}\n", n_route_unique);

    dense_set<Vec2u16> visited_points;
    std::array<Matrix<bool>, 4> visited_states;
    for (auto &v : visited_states)
        v = Matrix<bool>(grid.rows, grid.cols);
    walk2(grid, start, visited_states, &visited_points);

    int n_loops = 0;
    for (auto p : visited_points) {
        char c = grid(p);
        if (c == '.' || c == '^') {
            grid(p) = '#';
            n_loops += !walk2(grid, start, visited_states);
            grid(p) = c;
        }
    }
    fmt::print("{}\n", n_loops);
}

}
