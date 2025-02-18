#include "common.h"
#include "dense_set.h"

namespace aoc_2023_3 {

static boost::container::static_vector<Vec2z, 8>
surrounding_part_numbers(const Matrix<char> &grid, Vec2z p)
{
    boost::container::static_vector<Vec2z, 8> result;

    for (auto n : neighbors8(grid, p)) {
        if (isdigit(grid(n))) {
            while (n.x > 0 && isdigit(grid(n.y, n.x - 1)))
                n.x--;

            if (std::find(result.begin(), result.end(), n) == result.end())
                result.push_back(n);
        }
    }

    return result;
}

static int read_number(const Matrix<char> &grid, Vec2z p)
{
    int n = 0;
    for (auto [x, y] = p; x < grid.cols && isdigit(grid(y, x)); x++)
        n = 10 * n + grid(y, x) - '0';
    return n;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines);

    dense_set<Vec2z> part_number_positions;
    for (auto p : grid.ndindex()) {
        if (!isdigit(grid(p)) && grid(p) != '.') {
            for (auto n : surrounding_part_numbers(grid, p))
                part_number_positions.insert(n);
        }
    }

    int sum = 0;
    for (auto p : part_number_positions)
        sum += read_number(grid, p);
    fmt::print("{}\n", sum);

    int ratio_sum = 0;
    for (auto p : grid.ndindex()) {
        if (grid(p) == '*') {
            if (auto n = surrounding_part_numbers(grid, p); n.size() == 2)
                ratio_sum += read_number(grid, n[0]) * read_number(grid, n[1]);
        }
    }
    fmt::print("{}\n", ratio_sum);
}

}
