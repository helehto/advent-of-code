#include "common.h"
#include "dense_set.h"

namespace aoc_2022_18 {

using Cube = std::array<int, 3>;

static Cube with_offset(Cube c, int dim, int delta)
{
    c[dim] += delta;
    return c;
}

static int sum_surface_area(const dense_set<Cube, CrcHasher> &cubes)
{
    int sum = 0;
    for (auto &c : cubes) {
        int free = 6;
        for (int dim = 0; dim < 3; dim++) {
            for (int sign : {-1, 1}) {
                if (cubes.count(with_offset(c, dim, sign)))
                    free--;
            }
        }
        sum += free;
    }

    return sum;
}

static dense_set<Cube, CrcHasher> flood(dense_set<Cube, CrcHasher> occupied)
{
    int min[3] = {INT_MAX, INT_MAX, INT_MAX};
    int max[3] = {INT_MIN, INT_MIN, INT_MIN};

    for (auto &c : occupied) {
        for (int i = 0; i < 3; i++) {
            min[i] = std::min(min[i], c[i]);
            max[i] = std::max(max[i], c[i]);
        }
    }

    auto can_fill = [&](Cube c) {
        // Can't fill out of bounds
        for (int i = 0; i < 3; i++) {
            if (!(min[i] <= c[i] && c[i] <= max[i]))
                return false;
        }

        // Can only fill empty space, not lava cubes or water/steam
        return occupied.count(c) == 0;
    };

    std::vector<Cube> q{{min[0], min[1], min[2]}};
    while (!q.empty()) {
        auto u = q.back();
        q.pop_back();
        occupied.insert(u);

        for (int dim = 0; dim < 3; dim++) {
            for (int sign : {-1, 1}) {
                Cube v = with_offset(u, dim, sign);
                if (can_fill(v))
                    q.push_back(v);
            }
        }
    }

    dense_set<Cube, CrcHasher> unvisited;
    unvisited.reserve(2048);

    for (int x = min[0]; x < max[0]; x++) {
        for (int y = min[1]; y < max[1]; y++) {
            for (int z = min[2]; z < max[2]; z++) {
                Cube p = {x, y, z};
                if (!occupied.count(p))
                    unvisited.emplace(p);
            }
        }
    }

    return unvisited;
}

void run(std::string_view buf)
{
    const auto lines = split_lines(buf);
    dense_set<Cube, CrcHasher> cubes;
    cubes.reserve(lines.size());
    for (std::string_view line : lines)
        cubes.insert(find_numbers_n<int, 3>(line));

    const auto area = sum_surface_area(cubes);
    fmt::print("{}\n", area);

    const auto unvisited = flood(cubes);
    fmt::print("{}\n", area - sum_surface_area(unvisited));
}

}
