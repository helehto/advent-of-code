#include "common.h"
#include "dense_map.h"

namespace aoc_2024_8 {

void run(FILE *f)
{
    auto [_, lines] = slurp_lines(f);
    auto grid = Matrix<char>::from_lines(lines);

    dense_map<char, std::vector<Point<uint16_t>>> antennas;
    for (auto p : grid.ndindex<uint16_t>())
        if (grid(p) != '.' && grid(p) != '#')
            antennas[grid(p)].push_back(p);

    auto antinodes1 = grid;
    auto antinodes2 = grid;

    for (auto &[_, points] : antennas) {
        for (size_t i = 0; i < points.size(); i++) {
            for (size_t j = i + 1; j < points.size(); j++) {
                const auto p0 = points[i];
                const auto p1 = points[j];

                for (size_t k = 0; k < std::max(grid.rows, grid.cols); k++) {
                    const int dx = p1.x - p0.x;
                    const int dy = p1.y - p0.y;

                    if (Point p = p1.translate(-k * dx, -k * dy); grid.in_bounds(p)) {
                        if (k == 2)
                            antinodes1(p) = '#';
                        antinodes2(p) = '#';
                    }

                    if (Point p = p0.translate(k * dx, k * dy); grid.in_bounds(p)) {
                        if (k == 2)
                            antinodes1(p) = '#';
                        antinodes2(p) = '#';
                    }
                }
            }
        }
    }

    fmt::print("{}\n", std::ranges::count(antinodes1.all(), '#'));
    fmt::print("{}\n", std::ranges::count(antinodes2.all(), '#'));
}

}
