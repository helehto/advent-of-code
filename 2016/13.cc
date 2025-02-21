#include "common.h"
#include "dense_set.h"

namespace aoc_2016_13 {

void run(std::string_view buf)
{
    auto [n] = find_numbers_n<int, 1>(buf);

    std::vector<std::pair<int, Vec2i>> queue;
    queue.reserve(16'384);
    queue.emplace_back(0, Vec2i{1, 1});

    dense_set<Vec2i> visited;
    dense_set<Vec2i> visited_in_50_steps;

    for (size_t i = 0;; ++i) {
        auto [d, u] = queue[i];

        if (u == Vec2i{31, 39}) {
            fmt::print("{}\n", d);
            break;
        }
        if (d <= 50)
            visited_in_50_steps.insert(u);

        visited.insert(u);

        for (Vec2i v : neighbors4(u)) {
            if (v.x < 0 || v.y < 0)
                continue;

            uint32_t k = v.x * v.x + 3 * v.x + 2 * v.x * v.y + v.y + v.y * v.y + n;
            if (std::popcount(k) % 2 == 0 && !visited.count(v))
                queue.emplace_back(d + 1, v);
        }
    }

    fmt::print("{}\n", visited_in_50_steps.size());
}

}
