#include "common.h"
#include "dense_set.h"

namespace aoc_2024_6 {

enum { N, E, S, W };
constexpr int dx[] = {0, 1, 0, -1};
constexpr int dy[] = {-1, 0, 1, 0};

struct State {
    uint16_t x : 15;
    uint16_t y : 15;
    uint16_t dir : 2;

    constexpr bool operator==(const State &) const = default;

    struct Hasher {
        size_t operator()(const State &state) const
        {
            size_t h = 0;
            hash_combine(h, state.x, state.y, state.dir);
            return h;
        }
    };
};

static std::pair<std::vector<State>, size_t> walk1(const Matrix<char> &grid,
                                                   Point<uint16_t> p)
{
    uint16_t dir = N;
    std::vector<State> visited{State{p.x, p.y, dir}};
    dense_set<Point<uint16_t>> unique{p};

    while (true) {
        auto q = p.translate(dx[dir], dy[dir]);
        if (!grid.in_bounds(q))
            break;

        while (grid(q) == '#') {
            dir = (dir + 1) & 3;
            q = p.translate(dx[dir], dy[dir]);
        }

        visited.push_back(State{q.x, q.y, dir});
        unique.insert(q);
        p = q;
    }

    return std::pair(visited, unique.size());
}

static bool walk2(const Matrix<char> &grid,
                  Point<uint16_t> p,
                  dense_set<State, State::Hasher> &visited,
                  dense_set<Point<uint16_t>> *out = nullptr)
{
    uint16_t dir = N;
    visited.clear();
    visited.reserve(grid.rows * grid.cols);
    visited.insert(State{p.x, p.y, dir});

    while (true) {
        auto q = p.translate(dx[dir], dy[dir]);
        if (!grid.in_bounds(q))
            break;

        while (grid(q) == '#') {
            dir = (dir + 1) & 3;
            q = p.translate(dx[dir], dy[dir]);
        }

        if (!visited.insert(State{q.x, q.y, dir}).second)
            return false;

        p = q;
    }

    if (out) {
        out->clear();
        out->reserve(visited.size());
        for (State state : visited)
            out->insert({state.x, state.y});
    }

    return true;
}

void run(FILE *f)
{
    auto [_, lines] = slurp_lines(f);
    auto grid = Matrix<char>::from_lines(lines);

    Point<uint16_t> start{};
    for (auto p : grid.ndindex<uint16_t>()) {
        if (grid(p) == '^') {
            start = p;
            break;
        }
    }

    auto [route, n_route_unique] = walk1(grid, start);
    fmt::print("{}\n", n_route_unique);

    dense_set<Point<uint16_t>> visited_points;
    dense_set<State, State::Hasher> visited_states;
    walk2(grid, start, visited_states, &visited_points);
    fmt::print("{}\n", visited_points.size());

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
