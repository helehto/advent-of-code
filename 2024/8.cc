#include "common.h"
#include "dense_map.h"

namespace aoc_2024_8 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines);

    dense_map<char, small_vector<Vec2u16>> antennas;
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
                    if (Vec2 p = p1 - k * (p1 - p0); grid.in_bounds(p)) {
                        if (k == 2)
                            antinodes1(p) = '#';
                        antinodes2(p) = '#';
                    }

                    if (Vec2 p = p0 + k * (p1 - p0); grid.in_bounds(p)) {
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
