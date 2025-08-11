#include "common.h"
#include "dense_map.h"
#include "dense_set.h"

namespace aoc_2017_22 {

// NOTE: The specific order of the enumerators in both types below matters.
enum { CLEAN, WEAKENED, INFECTED, FLAGGED };
enum { N, E, S, W };

struct Grid {
    static constexpr int dense_size = 500;
    Matrix<int64_t> dense_grid{dense_size + 1, dense_size + 1, CLEAN};
    dense_map<Vec2i16, int64_t> sparse_grid;

    [[gnu::noinline, gnu::cold]] int64_t &lookup_sparse(const Vec2i pos) noexcept
    {
        return sparse_grid.emplace(pos.cast<int16_t>(), CLEAN).first->second;
    }

    [[gnu::noinline, gnu::cold]] int64_t &lookup_sparse(const Vec2z pos) noexcept
    {
        return sparse_grid.emplace(pos.cast<int16_t>(), CLEAN).first->second;
    }

    int64_t &operator[](const Vec2z pos, int64_t *k) noexcept
    {
        const size_t x = static_cast<size_t>(pos.x) + dense_size / 2;
        const size_t y = static_cast<size_t>(pos.y) + dense_size / 2;
        if (x < dense_grid.cols && y < dense_grid.rows) [[likely]]
            return *k;
        else
            return lookup_sparse(pos);
    }

    int64_t &operator[](const Vec2i pos) noexcept
    {
        const size_t x = static_cast<size_t>(pos.x) + dense_size / 2;
        const size_t y = static_cast<size_t>(pos.y) + dense_size / 2;
        if (x < dense_grid.cols && y < dense_grid.rows) [[likely]]
            return dense_grid(y, x);
        else
            return lookup_sparse(pos);
    }
};

static std::pair<dense_set<Vec2i>, Vec2i> parse_input(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines);

    dense_set<Vec2i> cells;
    cells.reserve(grid.rows * grid.cols);
    for (auto p : grid.ndindex<int>()) {
        if (grid(p) == '#') {
            grid(p) = INFECTED;
            cells.insert(p);
        } else {
            grid(p) = CLEAN;
        }
    }

    return {cells, Vec2i(grid.rows / 2, grid.cols / 2)};
}

[[gnu::noinline]] static int part1(dense_set<Vec2i> cells, const Vec2i start)
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

constexpr auto dir2dxdy = [] consteval {
    std::array<Vec2z, 4> result{};
    result[N] = {0, static_cast<size_t>(-1)};
    result[E] = {+1, 0};
    result[S] = {0, +1};
    result[W] = {static_cast<size_t>(-1), 0};
    return result;
}();

constexpr auto dir2linear = [] consteval {
    std::array<ptrdiff_t, 4> result{};
    result[N] = -Grid::dense_size - 1;
    result[E] = +1;
    result[S] = Grid::dense_size + 1;
    result[W] = -1;
    return result;
}();

[[gnu::noinline]] static int part2(const dense_set<Vec2i> &cells, const Vec2i start)
{
    size_t dir = N;
    std::array<int, 4> n_states{};

    Grid grid;
    for (const Vec2i &p : cells)
        grid[p] = INFECTED;

    Vec2z pos = start.cast<size_t>();
    int64_t *p = &grid[start];

    for (int i = 0; i < 10'000'000; ++i) {
        auto &state = grid[pos, p];
        dir += state - 1;
        n_states[state]++;
        state = (state + 1) & 3;
        pos += dir2dxdy[dir & 3];
        p += dir2linear[dir & 3];
    }

    return n_states[WEAKENED];
}

void run(std::string_view buf)
{
    auto [cells, start] = parse_input(buf);
    fmt::print("{}\n", part1(cells, start));
    fmt::print("{}\n", part2(cells, start));
}

}
