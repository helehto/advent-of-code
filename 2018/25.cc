#include "common.h"
#include "small_vector.h"

namespace aoc_2018_25 {

void run(std::string_view buf)
{
    const auto nums = find_numbers<int8_t>(buf);
    ASSERT(nums.size() % 4 == 0);
    const size_t n = nums.size() / 4;

    auto storage = std::make_unique_for_overwrite<int8_t[]>(4 * n);
    auto *xs = storage.get();
    auto *ys = xs + n;
    auto *zs = ys + n;
    auto *ws = zs + n;

    for (size_t i = 0; i < n; ++i) {
        xs[i] = nums[4 * i + 0];
        ys[i] = nums[4 * i + 1];
        zs[i] = nums[4 * i + 2];
        ws[i] = nums[4 * i + 3];
    }

    std::vector<int16_t> constellations(n, -1);
    std::vector<small_vector<uint16_t, 4>> neighbors(n);
    std::vector<int> offsets(n + 1);

    std::vector<int8_t> d(n);

    // Build the graph. Distance computation and edge creation are split into
    // separate loops to enable vectorization of the former.
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            d[j] = std::abs(xs[i] - xs[j]) + std::abs(ys[i] - ys[j]) +
                   std::abs(zs[i] - zs[j]) + std::abs(ws[i] - ws[j]);
        }

        for (size_t j = i + 1; j < n; ++j) {
            if (d[j] <= 3)
                neighbors[i].push_back(j);
        }
    }

    // Make the edges bidirectional:
    for (size_t i = 0; i < neighbors.size(); ++i) {
        for (size_t j : neighbors[i])
            neighbors[j].push_back(i);
    }

    // Cluster scanning:
    std::vector<uint16_t> queue;
    uint16_t num_constellations = 0;
    while (true) {
        auto it = std::ranges::find(constellations, -1);
        if (it == constellations.end())
            break;

        queue.clear();
        queue.push_back(it - constellations.begin());

        while (!queue.empty()) {
            auto i = queue.back();
            queue.pop_back();
            constellations[i] = num_constellations;

            for (auto j : neighbors[i]) {
                if (constellations[j] < 0) {
                    constellations[j] = num_constellations;
                    queue.push_back(j);
                }
            }
        }

        num_constellations++;
    }

    fmt::print("{}\n", num_constellations);
}

}
