#include "common.h"
#include <algorithm>
#include <ranges>
#include <unordered_set>

namespace aoc_2021_9 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto chart = Matrix<char>::from_lines(lines);

    int risk_sum = 0;
    for (auto p : chart.ndindex()) {
        char min_neighbor = CHAR_MAX;
        for (auto p : neighbors4(chart, p))
            min_neighbor = std::min(min_neighbor, chart(p));

        if (chart(p) < min_neighbor)
            risk_sum += chart(p) + 1 - '0';
    }
    fmt::print("{}\n", risk_sum);

    std::unordered_set<Vec2z> unvisited;
    for (auto p : chart.ndindex()) {
        if (chart(p) != '9')
            unvisited.insert(p);
    }

    Matrix<int> basins(chart.rows, chart.cols, -1);
    int next_basin = 0;
    std::vector<Vec2z> visited;

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
    for (auto b : basins.all()) {
        if (b >= 0)
            sizes[b]++;
    }
    std::sort(begin(sizes), end(sizes), std::greater<int>());
    fmt::print("{}\n", sizes[0] * sizes[1] * sizes[2]);
}

}
