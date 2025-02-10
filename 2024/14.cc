#include "common.h"

namespace aoc_2024_14 {

struct Robots {
    std::vector<int16_t> x;
    std::vector<int16_t> y;
    std::vector<int16_t> vx;
    std::vector<int16_t> vy;
    int w;
    int h;
};

static void step(Robots &r)
{
    const size_t n = r.x.size();

    // Add velocity to position. The loops are manually split between x/y, and
    // wrap around is handled separately below, to make this amenable to
    // autovectorization.
    for (size_t i = 0; i < n; ++i)
        r.x[i] += r.vx[i];
    for (size_t i = 0; i < n; ++i)
        r.y[i] += r.vy[i];

    // Wrap around. Avoid modulo to avoid emitting a division instruction and
    // to make this autovectorizable.
    for (size_t i = 0; i < n; ++i) {
        if (r.x[i] < 0)
            r.x[i] += r.w;
        else if (r.x[i] >= r.w)
            r.x[i] -= r.w;
    }
    for (size_t i = 0; i < n; ++i) {
        if (r.y[i] < 0)
            r.y[i] += r.h;
        else if (r.y[i] >= r.h)
            r.y[i] -= r.h;
    }
}

static int safety_factor(const Robots &r)
{
    int q[4]{};
    for (size_t i = 0; i < r.x.size(); ++i) {
        if (r.x[i] < r.w / 2 && r.y[i] < r.h / 2)
            q[0]++;
        else if (r.x[i] < r.w / 2 && r.y[i] > r.h / 2)
            q[1]++;
        else if (r.x[i] > r.w / 2 && r.y[i] > r.h / 2)
            q[2]++;
        else if (r.x[i] > r.w / 2 && r.y[i] < r.h / 2)
            q[3]++;
    }
    return q[0] * q[1] * q[2] * q[3];
}

static bool has_tree(const Robots &r)
{
    uint8_t count[103]{};

    for (auto x : r.x) {
        count[x]++;
        if (count[x] <= 30)
            continue;

        std::vector<int> ys;
        ys.reserve(r.x.size());
        for (size_t i = 0; i < r.x.size(); ++i) {
            if (r.x[i] == x)
                ys.push_back(r.y[i]);
        }

        std::ranges::sort(ys);

        size_t max_streak = 0;
        size_t current_streak = 0;
        for (size_t i = 1; i < ys.size(); ++i) {
            if (ys[i] - ys[i - 1] <= 1) {
                current_streak++;
            } else {
                max_streak = std::max(max_streak, current_streak + 1);
                current_streak = 0;
            }
        }
        max_streak = std::max(max_streak, current_streak + 1);

        if (max_streak > 21)
            return true;
    }

    return false;
}

void run(FILE *f)
{
    auto buf = slurp(f);
    std::vector<int> nums;
    find_numbers(buf, nums);

    Robots r{
        .w = nums.size() >= 120 ? 101 : 11,
        .h = nums.size() >= 120 ? 103 : 7,
    };
    for (size_t i = 0; i < nums.size(); i += 4) {
        r.x.push_back(nums[i + 0]);
        r.y.push_back(nums[i + 1]);
        r.vx.push_back(nums[i + 2]);
        r.vy.push_back(nums[i + 3]);
    }

    size_t i = 0;
    for (; i < 100; ++i)
        step(r);
    fmt::print("{}\n", safety_factor(r));

    for (; !has_tree(r); i++)
        step(r);
    fmt::print("{}\n", i);
}

}
