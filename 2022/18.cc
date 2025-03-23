#include "common.h"
#include <climits>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

template <>
struct std::hash<std::array<int, 3>> {
    size_t operator()(const std::array<int, 3> a) const
    {
        uint64_t u;
        memcpy(&u, &a, sizeof(u));
        return _mm_crc32_u64(_mm_crc32_u64(0, u), a[2]);
    }
};

namespace aoc_2022_18 {

using Cube = std::array<int, 3>;

static Cube with_offset(Cube c, int dim, int delta)
{
    c[dim] += delta;
    return c;
}

static int sum_surface_area(const std::unordered_set<Cube> &cubes)
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

static std::unordered_set<Cube> flood(std::unordered_set<Cube> occupied)
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

    std::unordered_set<Cube> unvisited;

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
    std::unordered_set<Cube> cubes;
    for (std::string_view line : split_lines(buf))
        cubes.insert(find_numbers_n<int, 3>(line));

    const auto area = sum_surface_area(cubes);
    fmt::print("{}\n", area);

    const auto unvisited = flood(cubes);
    fmt::print("{}\n", area - sum_surface_area(unvisited));
}

}
