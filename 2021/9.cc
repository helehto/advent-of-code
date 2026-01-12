#include "common.h"

namespace aoc_2021_9 {

static int part1(MatrixView<const char> g)
{
    int result = 0;
    const size_t cols = g.cols;

    for (size_t i = 1; i < g.rows - 1; i++) {
        const char *p = &g(i, 1);
        for (size_t j = 1; j < g.cols - 1; j++, p++) {
            char min_neighbor = std::min({p[-cols], p[+cols], p[-1], p[+1]});
            result += *p < min_neighbor ? *p + 1 - '0' : 0;
        }
    }

    return result;
}

static int part2(MatrixView<const char> g)
{
    const size_t cols = g.cols;
    const char *data = g.data();

    std::vector<uint8_t> unvisited(g.size(), 1);
    std::vector<int16_t> basins(g.size(), -1);
    int next_basin = 0;
    std::vector<uint16_t> queue;

    std::vector<uint16_t> seeds;
    for (size_t offset = 0; offset < g.size() - 1; offset++)
        if (data[offset] >= '9' && data[offset + 1] < '9')
            seeds.push_back(static_cast<uint16_t>(offset + 1));

    for (uint16_t seed : seeds) {
        queue = {seed};
        while (!queue.empty()) {
            size_t offset = queue.back();
            queue.pop_back();

            if (basins[offset] >= 0)
                continue;

            const char *row = &data[offset];
            const char *above = row - cols;
            const char *below = row + cols;
            const int16_t *basins_above = &basins[offset - cols];
            const int16_t *basins_below = &basins[offset + cols];

            if (above[-1] < '9' && above[0] < '9' && basins_above[0] < 0)
                queue.push_back(&above[-1] - data);
            if (below[-1] < '9' && below[0] < '9' && basins_below[0] < 0)
                queue.push_back(&below[-1] - data);

            for (size_t i = 0; row[i] < '9'; ++i) {
                if (above[i - 1] >= '9' && above[i] < '9' && basins_above[i] < 0)
                    queue.push_back(&above[i] - data);
                if (below[i - 1] >= '9' && below[i] < '9' && basins_below[i] < 0)
                    queue.push_back(&below[i] - data);

                basins[offset + i] = next_basin;
            }
        }
        next_basin++;
    }

    std::vector<int> sizes(next_basin);
    for (auto b : basins)
        if (b >= 0)
            sizes[b]++;

    std::ranges::nth_element(sizes, sizes.begin() + 2, λab(a > b));
    return sizes[0] * sizes[1] * sizes[2];
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines).padded(1, '~');
    fmt::print("{}\n", part1(grid));
    fmt::print("{}\n", part2(grid));
}

}
