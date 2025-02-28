#include "common.h"

namespace aoc_2016_24 {

void run(std::string_view buf)
{
    std::vector<Vec2i> locations;
    locations.reserve(10);

    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines);

    for (Vec2i p : grid.ndindex<int>()) {
        size_t n = grid(p) - '0';
        if (n <= 9) {
            if (locations.size() <= n)
                locations.resize(n + 1, Vec2i{});
            locations[n] = p;
        }
    }

    Matrix<int> distances(locations.size(), locations.size(), -1);
    for (size_t i = 0; i < locations.size(); ++i) {
        std::vector<std::pair<int, Vec2i>> queue;
        Matrix<bool> visited(grid.rows, grid.cols, false);

        queue.emplace_back(0, locations[i]);

        for (size_t j = 0; j < queue.size(); ++j) {
            const auto [d, u] = queue[j];

            if (visited(u))
                continue;
            visited(u) = true;

            const int k = grid(u) - '0';
            if (k >= 0 && k <= 9 && distances(i, k) < 0) {
                distances(i, k) = d;
                distances(k, i) = d;
            }

            for (Vec2i v : neighbors4(grid, u)) {
                if (grid(v) != '#' && !visited(v))
                    queue.emplace_back(d + 1, v);
            }
        }
    }

    auto solve = [&](std::vector<int> order_, bool loop) {
        auto order = order_;
        int result = INT_MAX;
        do {
            int total_dist = 0;
            for (size_t i = 1; i < order.size(); ++i)
                total_dist += distances(order[i - 1], order[i]);
            if (loop)
                total_dist += distances(0, order.back());
            result = std::min(result, total_dist);
        } while (std::next_permutation(order.begin() + 1, order.end()));
        return result;
    };

    std::vector<int> order;
    for (size_t i = 0; i < locations.size(); ++i)
        order.push_back(i);
    fmt::print("{}\n", solve(order, false));
    fmt::print("{}\n", solve(order, true));
}
}
