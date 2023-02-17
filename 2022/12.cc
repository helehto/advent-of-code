#include "common.h"
#include <algorithm>
#include <cassert>
#include <climits>
#include <fmt/core.h>
#include <string>
#include <vector>

static int dijkstra(const Matrix<char> &m, size_t target, std::vector<int> &dist)
{
    auto cmp = [&](size_t a, size_t b) { return dist[a] < dist[b]; };
    BinaryHeap q(cmp, m.size());
    std::vector<size_t> neighbors;

    neighbors.reserve(4);

    while (!q.empty()) {
        auto u = q.top();
        q.pop();

        const size_t x = u % m.cols;
        const size_t y = u / m.cols;

        neighbors.clear();
        if (x > 0)
            neighbors.emplace_back(u - 1);
        if (x + 1 < m.cols)
            neighbors.emplace_back(u + 1);
        if (y > 0)
            neighbors.emplace_back(u - m.cols);
        if (y + 1 < m.rows)
            neighbors.emplace_back(u + m.cols);

        for (size_t v : neighbors) {
            if (m.data[v] > m.data[u] + 1)
                continue;

            auto new_dist = dist[u] + 1;
            if (new_dist < dist[v]) {
                dist[v] = new_dist;
                q.decrease(v);
            }
        }
    }

    return dist[target];
}

void run_2022_12(FILE *f)
{
    const auto lines = getlines(f);
    Matrix<char> m(lines.size(), lines[0].size());
    for (size_t i = 0; i < lines.size(); i++) {
        for (size_t j = 0; j < lines[0].size(); j++) {
            m(i, j) = lines[i][j];
        }
    }

    // Find the start and end point (at least for my input, the start point
    // isn't at the top left as in the example).
    size_t start_index = 0, end_index = 0;
    for (size_t i = 0; i < m.size(); i++) {
        if (m.data[i] == 'S')
            start_index = i;
        else if (m.data[i] == 'E')
            end_index = i;
    }
    m.data[start_index] = 'a';
    m.data[end_index] = 'z';

    // Part 1:
    {
        std::vector<int> dist(m.size(), 100000);
        dist[start_index] = 0;
        fmt::print("{}\n", dijkstra(m, end_index, dist));
    }

    // Part 2:
    {
        std::vector<int> dist(m.size(), 100000);

        // Set the distance of all 'a' points to 0. In effect, this causes
        // Dijkstra's algorithm to consider them all as starting points
        // simultaneously.
        for (size_t i = 0; i < m.size(); i++) {
            if (m.data[i] == 'a')
                dist[i] = 0;
        }

        fmt::print("{}\n", dijkstra(m, end_index, dist));
    }
}
