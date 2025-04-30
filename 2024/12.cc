#include "common.h"
#include "dense_map.h"

namespace aoc_2024_12 {

static std::vector<small_vector<Vec2i>> regions(const Matrix<char> &g,
                                                const std::vector<Vec2i> &seeds)
{
    Matrix<bool> visited(g.rows, g.cols, false);
    small_vector<Vec2i, 256> stack;

    auto flood = [&](Vec2i start, small_vector_base<Vec2i> &result) {
        stack = {start};

        while (!stack.empty()) {
            auto u = stack.back();
            stack.pop_back();
            if (!std::exchange(visited(u), true)) {
                result.push_back(u);
                for (auto v : neighbors4(g, u))
                    if (g(u) == g(v) && !visited(v))
                        stack.push_back(v);
            }
        }
    };

    std::vector<small_vector<Vec2i>> result;
    result.reserve(seeds.size());
    for (auto p : seeds)
        if (!visited(p))
            flood(p, result.emplace_back());
    return result;
}

static int region_perimeter(const Matrix<char> &g, std::span<const Vec2i> points)
{
    int result = 0;
    for (auto u : points)
        for (auto v : neighbors4(u))
            result += !g.in_bounds(v) || g(u) != g(v);
    return result;
}

static int region_sides(const Matrix<char> &g, std::span<const Vec2i> points)
{
    constexpr Vec2i d[] = {{-1, 0}, {+1, 0}, {0, -1}, {0, +1}};
    Matrix<char> mask(g.rows, g.cols);
    int result = 0;
    std::vector<Vec2i> q;

    for (size_t i = 0; i < 4; i++) {
        std::ranges::fill(mask.all(), false);

        q.clear();
        for (auto u : points) {
            if (auto v = u + d[i]; !g.in_bounds(v) || g(u) != g(v)) {
                mask(u) = true;
                q.push_back(u);
            }
        }

        result += regions(mask, q).size();
    }

    return result;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto g = Matrix<char>::from_lines(lines);

    std::vector<Vec2i> points;
    points.reserve(g.rows * g.cols);
    for (auto p : g.ndindex<int>())
        points.push_back(p);

    auto ccs = regions(g, points);

    int s1 = 0;
    int s2 = 0;
    for (const auto &r : ccs) {
        s1 += r.size() * region_perimeter(g, r);
        s2 += r.size() * region_sides(g, r);
    }

    fmt::print("{}\n", s1);
    fmt::print("{}\n", s2);
}

}
