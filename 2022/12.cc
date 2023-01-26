#include "common.h"
#include <algorithm>
#include <cassert>
#include <climits>
#include <fmt/core.h>
#include <string>
#include <vector>

struct Map {
    std::string height;
    size_t cols;
    size_t rows;

    std::pair<size_t, size_t> index_to_point(size_t i) const
    {
        return {i % cols, i / cols};
    }
};

static int dijkstra(Map &m, size_t target, std::vector<int> &dist)
{
    auto cmp = [&](size_t a, size_t b) { return dist[a] < dist[b]; };
    BinaryHeap q(cmp, m.height.size());
    std::vector<size_t> neighbors;

    neighbors.reserve(4);

    while (!q.empty()) {
        auto u = q.top();
        q.pop();

        const auto [x, y] = m.index_to_point(u);

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
            if (m.height[v] > m.height[u] + 1)
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
    Map m{};
    std::string s;

    while (getline(f, s)) {
        assert(m.cols == 0 || m.cols == s.size());
        m.height += s;
        m.cols = s.size();
    }
    assert(m.height.size() % m.cols == 0);
    m.rows = m.height.size() / m.cols;

    // Find the start and end point (at least for my input, the start point
    // isn't at the top left as in the example).
    size_t start_index = 0, end_index = 0;
    for (size_t i = 0; i < m.height.size(); i++) {
        if (m.height[i] == 'S')
            start_index = i;
        else if (m.height[i] == 'E')
            end_index = i;
    }
    m.height[start_index] = 'a';
    m.height[end_index] = 'z';

    // Part 1:
    {
        std::vector<int> dist(m.height.size(), 100000);
        dist[start_index] = 0;
        fmt::print("{}\n", dijkstra(m, end_index, dist));
    }

    // Part 2:
    {
        std::vector<int> dist(m.height.size(), 100000);

        // Set the distance of all 'a' points to 0. In effect, this causes
        // Dijkstra's algorithm to consider them all as starting points
        // simultaneously.
        for (size_t i = 0; i < m.height.size(); i++) {
            if (m.height[i] == 'a')
                dist[i] = 0;
        }

        fmt::print("{}\n", dijkstra(m, end_index, dist));
    }
}
