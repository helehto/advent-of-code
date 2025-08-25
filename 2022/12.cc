#include "common.h"
#include "monotonic_bucket_queue.h"

namespace aoc_2022_12 {

static int dijkstra(MatrixView<const char> m, size_t target, std::vector<uint32_t> &dist)
{
    MonotonicBucketQueue<uint32_t> bq(2);

    for (size_t i = 0; i < m.size(); ++i)
        if (m.data()[i] == 'a')
            bq.emplace(0, i);

    while (std::optional<uint32_t> u = bq.pop()) {
        if (dist[*u] != bq.current_priority())
            continue;

        auto expand = [&](size_t v) {
            if (m.data()[v] <= m.data()[*u] + 1) {
                if (auto new_dist = dist[*u] + 1; new_dist < dist[v]) {
                    dist[v] = new_dist;
                    bq.emplace(new_dist, v);
                }
            }
        };

        const size_t x = *u % m.cols;
        if (x > 0)
            expand(*u - 1);
        if (x + 1 < m.cols)
            expand(*u + 1);
        if (*u >= m.cols)
            expand(*u - m.cols);
        if (*u + m.cols < m.size())
            expand(*u + m.cols);
    }

    return dist[target];
}

void run(std::string_view buf)
{
    auto m = Matrix<char>::from_lines(split_lines(buf));

    // Find the start and end point (at least for my input, the start point
    // isn't at the top left as in the example).
    size_t start_index = std::ranges::find(m.all(), 'S') - m.all().begin();
    size_t end_index = std::ranges::find(m.all(), 'E') - m.all().begin();
    m.data()[start_index] = 'a';
    m.data()[end_index] = 'z';

    std::vector<uint32_t> dist(m.size(), 100000);

    // Part 1:
    dist[start_index] = 0;
    fmt::print("{}\n", dijkstra(m, end_index, dist));

    // Part 2:
    {
        dist.assign(m.size(), 100000);

        // Set the distance of all 'a' points to 0. In effect, this causes
        // Dijkstra's algorithm to consider them all as starting points
        // simultaneously.
        for (size_t i = 0; i < m.size(); i++) {
            if (m.data()[i] == 'a')
                dist[i] = 0;
        }

        fmt::print("{}\n", dijkstra(m, end_index, dist));
    }
}

}
