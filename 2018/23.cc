#include "common.h"
#include "monotonic_bucket_queue.h"

namespace aoc_2018_23 {

struct Cube {
    int x;
    int y;
    int z;
    int width;

    constexpr std::array<Cube, 8> split() const
    {
        return {
            Cube{x, y, z, width / 2},
            Cube{x, y, z + width / 2, width / 2},
            Cube{x, y + width / 2, z, width / 2},
            Cube{x, y + width / 2, z + width / 2, width / 2},
            Cube{x + width / 2, y, z, width / 2},
            Cube{x + width / 2, y, z + width / 2, width / 2},
            Cube{x + width / 2, y + width / 2, z, width / 2},
            Cube{x + width / 2, y + width / 2, z + width / 2, width / 2},
        };
    }
};

struct Nanobots {
    std::unique_ptr<int[]> storage;
    size_t count;
    int *x;
    int *y;
    int *z;
    int *r;

    Nanobots(std::span<const int> nums)
        : storage(std::make_unique_for_overwrite<int[]>(nums.size()))
        , count(nums.size() / 4)
        , x(storage.get())
        , y(x + count)
        , z(y + count)
        , r(z + count)
    {
        for (size_t i = 0; i < count; ++i) {
            x[i] = nums[4 * i + 0];
            y[i] = nums[4 * i + 1];
            z[i] = nums[4 * i + 2];
            r[i] = nums[4 * i + 3];
        }
    }
};

constexpr int manhattan3(int dx, int dy, int dz)
{
    return std::abs(dx) + std::abs(dy) + std::abs(dz);
}

constexpr int count_overlaps(const Cube &cube, const Nanobots &bots)
{
    int result = 0;

    for (size_t i = 0; i < bots.count; ++i) {
        const auto cx = std::clamp(bots.x[i], cube.x, cube.x + cube.width - 1);
        const auto cy = std::clamp(bots.y[i], cube.y, cube.y + cube.width - 1);
        const auto cz = std::clamp(bots.z[i], cube.z, cube.z + cube.width - 1);
        const auto d = manhattan3(bots.x[i] - cx, bots.y[i] - cy, bots.z[i] - cz);
        result += (d <= bots.r[i]);
    }

    return result;
}

constexpr size_t part1(const Nanobots &bots)
{
    int result = 0;

    size_t max = 0;
    for (size_t i = 1; i < bots.count; ++i)
        if (bots.r[i] > bots.r[max])
            max = i;

    for (size_t i = 0; i < bots.count; ++i) {
        const auto d = manhattan3(bots.x[max] - bots.x[i], bots.y[max] - bots.y[i],
                                  bots.z[max] - bots.z[i]);
        result += (d <= bots.r[max]);
    }

    return result;
}

constexpr Cube bounding_cube(const Nanobots &bots)
{
    int xmin = INT_MAX, xmax = INT_MIN;
    int ymin = INT_MAX, ymax = INT_MIN;
    int zmin = INT_MAX, zmax = INT_MIN;
    for (size_t i = 0; i < bots.count; ++i) {
        xmin = std::min(xmin, bots.x[i]);
        xmax = std::max(xmax, bots.x[i]);
        ymin = std::min(ymin, bots.y[i]);
        ymax = std::max(ymax, bots.y[i]);
        zmin = std::min(zmin, bots.z[i]);
        zmax = std::max(zmax, bots.z[i]);
    }

    const unsigned width = std::max({xmax - xmin, ymax - ymin, zmax - zmin});
    return Cube(xmin, ymin, zmin, std::bit_ceil(width));
}

static int part2(const Nanobots &bots)
{
    MonotonicBucketQueue<Cube, small_vector<Cube, 32>> queue(bots.count);
    queue.emplace(0, bounding_cube(bots));

    std::optional<size_t> maximum;
    small_vector<std::array<int, 3>> maxarg;
    while (auto cube = queue.pop()) {
        auto weight = bots.count - queue.current_priority();
        if (maximum && weight < *maximum)
            break;

        if (cube->width == 1) {
            maximum = maximum.value_or(weight);
            maxarg.push_back({cube->x, cube->y, cube->z});
        } else {
            for (const Cube &subcube : cube->split()) {
                if (auto new_weight = count_overlaps(subcube, bots))
                    queue.emplace(bots.count - new_weight, subcube);
            }
        }
    }

    auto f = λa(manhattan3(a[0], a[1], a[2]));
    return std::ranges::max(maxarg | std::ranges::views::transform(f));
}

void run(std::string_view buf)
{
    const Nanobots bots(find_numbers<int>(buf));
    fmt::print("{}\n", part1(bots));
    fmt::print("{}\n", part2(bots));
}

}
