#include "common.h"
#include <algorithm>
#include <boost/container/static_vector.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <ranges>
#include <span>
#include <unordered_set>
#include <vector>

template <typename T, size_t N>
using static_vector = boost::container::static_vector<T, N>;

static std::array<Point<size_t>, 4> neighbors4(Point<size_t> p)
{
    return {{
        {p.x, p.y - 1},
        {p.x + 1, p.y},
        {p.x, p.y + 1},
        {p.x - 1, p.y},
    }};
}

static static_vector<Point<size_t>, 4>
neighbors4(const Matrix<char> &chart, Point<size_t> p)
{
    static_vector<Point<size_t>, 4> result;
    for (auto n : neighbors4(p))
        if (n.x < chart.cols && n.y < chart.rows)
            result.push_back(n);

    return result;
}

void run_2021_9(FILE *f)
{
    auto lines = getlines(f);

    Matrix<char> chart(lines.size(), lines[0].size());
    for (auto p : chart.ndindex())
        chart(p) = lines[p.y][p.x];

    int risk_sum = 0;
    for (auto p : chart.ndindex()) {
        char min_neighbor = CHAR_MAX;
        for (auto p : neighbors4(chart, p))
            min_neighbor = std::min(min_neighbor, chart(p));

        if (chart(p) < min_neighbor)
            risk_sum += chart(p) + 1 - '0';
    }
    fmt::print("{}\n", risk_sum);

    std::unordered_set<Point<size_t>> unvisited;
    for (auto p : chart.ndindex()) {
        if (chart(p) != '9')
            unvisited.insert(p);
    }

    Matrix<int> basins(chart.rows, chart.cols, -1);
    int next_basin = 0;
    std::vector<Point<size_t>> visited;

    while (!unvisited.empty()) {
        auto p = *unvisited.begin();

        auto basin = next_basin;
        visited.clear();

        // Wander to the/a lowest point in this basin.
        while (true) {
        restart_basin:
            if (unvisited.erase(p) == 0) {
                // This point is already part of an existing basin; any points
                // that we previously visited in this loop should be assigned
                // to the same basin.
                basin = basins(p);
                break;
            }

            // -2 for the currently visited line.
            basins(p) = -2;
            visited.push_back(p);

            for (auto n : neighbors4(chart, p)) {
                if (chart(n) < chart(p) && basins(n) != -2 && chart(n) != '9') {
                    p = n;
                    goto restart_basin;
                }
            }

            // This is either a low point, or we have visited all
            // neighbors.
            break;
        }

        for (auto v : visited)
            basins(v) = basin;
        if (basin == next_basin)
            next_basin++;
    }

    std::vector<int> sizes(next_basin);
    for (auto b : basins) {
        if (b >= 0)
            sizes[b]++;
    }
    std::sort(begin(sizes), end(sizes), std::greater<int>());
    fmt::print("{}\n", sizes[0] * sizes[1] * sizes[2]);
}
