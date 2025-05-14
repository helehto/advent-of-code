#include "common.h"
#include "dense_set.h"

namespace aoc_2020_17 {

struct Point4D : public std::array<int, 4> {
    constexpr bool operator==(const Point4D &other) const = default;
    constexpr bool operator!=(const Point4D &other) const = default;
};

}

template <>
struct std::hash<aoc_2020_17::Point4D> {
    constexpr size_t operator()(const aoc_2020_17::Point4D &p) const noexcept
    {
        size_t h = 0;
        for (int c : p)
            hash_combine(h, c);
        return h;
    }
};

namespace aoc_2020_17 {

static constexpr Point4D neighbors3d[] = {
    {-1, -1, -1, +0}, {-1, -1, +0, +0}, {-1, -1, +1, +0}, {-1, +0, -1, +0},
    {-1, +0, +0, +0}, {-1, +0, +1, +0}, {-1, +1, -1, +0}, {-1, +1, +0, +0},
    {-1, +1, +1, +0}, {+0, -1, -1, +0}, {+0, -1, +0, +0}, {+0, -1, +1, +0},
    {+0, +0, -1, +0}, {+0, +0, +1, +0}, {+0, +1, -1, +0}, {+0, +1, +0, +0},
    {+0, +1, +1, +0}, {+1, -1, -1, +0}, {+1, -1, +0, +0}, {+1, -1, +1, +0},
    {+1, +0, -1, +0}, {+1, +0, +0, +0}, {+1, +0, +1, +0}, {+1, +1, -1, +0},
    {+1, +1, +0, +0}, {+1, +1, +1, +0},
};

static constexpr Point4D neighbors4d[] = {
    {-1, -1, -1, -1}, {-1, -1, -1, +0}, {-1, -1, -1, +1}, {-1, -1, +0, -1},
    {-1, -1, +0, +0}, {-1, -1, +0, +1}, {-1, -1, +1, -1}, {-1, -1, +1, +0},
    {-1, -1, +1, +1}, {-1, +0, -1, -1}, {-1, +0, -1, +0}, {-1, +0, -1, +1},
    {-1, +0, +0, -1}, {-1, +0, +0, +0}, {-1, +0, +0, +1}, {-1, +0, +1, -1},
    {-1, +0, +1, +0}, {-1, +0, +1, +1}, {-1, +1, -1, -1}, {-1, +1, -1, +0},
    {-1, +1, -1, +1}, {-1, +1, +0, -1}, {-1, +1, +0, +0}, {-1, +1, +0, +1},
    {-1, +1, +1, -1}, {-1, +1, +1, +0}, {-1, +1, +1, +1}, {+0, -1, -1, -1},
    {+0, -1, -1, +0}, {+0, -1, -1, +1}, {+0, -1, +0, -1}, {+0, -1, +0, +0},
    {+0, -1, +0, +1}, {+0, -1, +1, -1}, {+0, -1, +1, +0}, {+0, -1, +1, +1},
    {+0, +0, -1, -1}, {+0, +0, -1, +0}, {+0, +0, -1, +1}, {+0, +0, +0, -1},
    {+0, +0, +0, +1}, {+0, +0, +1, -1}, {+0, +0, +1, +0}, {+0, +0, +1, +1},
    {+0, +1, -1, -1}, {+0, +1, -1, +0}, {+0, +1, -1, +1}, {+0, +1, +0, -1},
    {+0, +1, +0, +0}, {+0, +1, +0, +1}, {+0, +1, +1, -1}, {+0, +1, +1, +0},
    {+0, +1, +1, +1}, {+1, -1, -1, -1}, {+1, -1, -1, +0}, {+1, -1, -1, +1},
    {+1, -1, +0, -1}, {+1, -1, +0, +0}, {+1, -1, +0, +1}, {+1, -1, +1, -1},
    {+1, -1, +1, +0}, {+1, -1, +1, +1}, {+1, +0, -1, -1}, {+1, +0, -1, +0},
    {+1, +0, -1, +1}, {+1, +0, +0, -1}, {+1, +0, +0, +0}, {+1, +0, +0, +1},
    {+1, +0, +1, -1}, {+1, +0, +1, +0}, {+1, +0, +1, +1}, {+1, +1, -1, -1},
    {+1, +1, -1, +0}, {+1, +1, -1, +1}, {+1, +1, +0, -1}, {+1, +1, +0, +0},
    {+1, +1, +0, +1}, {+1, +1, +1, -1}, {+1, +1, +1, +0}, {+1, +1, +1, +1},
};

static std::pair<std::array<int, 4>, std::array<int, 4>>
grid_active_minmax(const dense_set<Point4D> &active)
{
    std::array<int, 4> min, max;
    min.fill(INT_MAX);
    max.fill(INT_MIN);

    for (Point4D p : active) {
        min[0] = std::min(min[0], p[0]);
        min[1] = std::min(min[1], p[1]);
        min[2] = std::min(min[2], p[2]);
        min[3] = std::min(min[3], p[3]);
        max[0] = std::max(max[0], p[0]);
        max[1] = std::max(max[1], p[1]);
        max[2] = std::max(max[2], p[2]);
        max[3] = std::max(max[3], p[3]);
    }

    return std::pair(min, max);
}

static bool
is_active(const dense_set<Point4D> &prev, const Point4D &p, const int active_neighbours)
{
    return prev.count(p) ? active_neighbours == 2 || active_neighbours == 3
                         : active_neighbours == 3;
}

static void step3d(dense_set<Point4D> &next_active, dense_set<Point4D> &prev_active)
{
    next_active.clear();
    auto [min, max] = grid_active_minmax(prev_active);

    for (int x = min[0] - 1; x <= max[0] + 1; x++) {
        for (int y = min[1] - 1; y <= max[1] + 1; y++) {
            for (int z = min[2] - 1; z <= max[2] + 1; z++) {
                const Point4D p{x, y, z, 0};

                int active_neighbours = 0;
                for (const Point4D d : neighbors3d) {
                    Point4D q{x + d[0], y + d[1], z + d[2], 0};
                    if (prev_active.count(q))
                        active_neighbours++;
                }

                if (is_active(prev_active, p, active_neighbours))
                    next_active.insert(p);
            }
        }
    }
}

static void step4d(dense_set<Point4D> &next_active, dense_set<Point4D> &prev_active)
{
    next_active.clear();
    auto [min, max] = grid_active_minmax(prev_active);

    for (int x = min[0] - 1; x <= max[0] + 1; x++) {
        for (int y = min[1] - 1; y <= max[1] + 1; y++) {
            for (int z = min[2] - 1; z <= max[2] + 1; z++) {
                for (int w = min[3] - 1; w <= max[3] + 1; w++) {
                    const Point4D p{x, y, z, w};

                    int active_neighbours = 0;
                    for (const Point4D d : neighbors4d) {
                        Point4D q{x + d[0], y + d[1], z + d[2], w + d[3]};
                        if (prev_active.count(q))
                            active_neighbours++;
                    }

                    if (is_active(prev_active, p, active_neighbours))
                        next_active.insert(p);
                }
            }
        }
    }
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines);
    dense_set<Point4D> tmp;

    {
        dense_set<Point4D> active;
        for (auto p : grid.ndindex<int>()) {
            if (grid(p) == '#')
                active.insert(Point4D{{p.x, p.y, 0, 0}});
        }

        for (size_t i = 0; i < 6; ++i) {
            step3d(tmp, active);
            tmp.swap(active);
        }
        fmt::print("{}\n", active.size());
    }

    {
        dense_set<Point4D> active;
        for (auto p : grid.ndindex<int>()) {
            if (grid(p) == '#')
                active.insert(Point4D{{p.x, p.y, 0, 0}});
        }

        for (size_t i = 0; i < 6; ++i) {
            step4d(tmp, active);
            tmp.swap(active);
        }
        fmt::print("{}\n", active.size());
    }
}

}
