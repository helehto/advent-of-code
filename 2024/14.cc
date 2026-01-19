#include "common.h"
#include "thread_pool.h"

namespace aoc_2024_14 {

struct DivideFixed {
    int16_t d;
    uint16_t multiplier;
    uint8_t shift;

    DivideFixed() = default;

    DivideFixed(int16_t d)
        : d(d)
    {
        // FIXME: Hard-coded shift, but should work for all reasonable inputs.
        shift = 16 + 3;
        multiplier = (UINT32_C(1) << shift) / d;
    }

    /// Compute `n % d`.
    constexpr int16_t modulo(int16_t n) const
    {
        int16_t q = (static_cast<int32_t>(n) * multiplier) >> shift;
        int16_t r = n - q * d;
        return r != d ? r : 0;
    }
};

struct Robots {
    std::vector<int16_t> x;
    std::vector<int16_t> y;
    std::vector<int16_t> vx;
    std::vector<int16_t> vy;
    int w;
    int h;
    DivideFixed w_div;
    DivideFixed h_div;

    void step(int rounds = 1)
    {
        const size_t n = x.size();

        // The loops here are split between x/y and a pre-computed
        // multiply+shift is used for the modulo operation to make this
        // amenable to autovectorization.
        for (size_t i = 0; i < n; ++i)
            x[i] = w_div.modulo(x[i] + rounds * vx[i]);
        for (size_t i = 0; i < n; ++i)
            y[i] = h_div.modulo(y[i] + rounds * vy[i]);
    }
};

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

void run(std::string_view buf)
{
    std::vector<int> nums;
    find_numbers(buf, nums);

    Robots r;
    r.w = nums.size() >= 120 ? 101 : 11;
    r.h = nums.size() >= 120 ? 103 : 7;
    r.w_div = DivideFixed(r.w);
    r.h_div = DivideFixed(r.h);

    for (size_t i = 0; i < nums.size(); i += 4) {
        r.x.push_back(nums[i + 0]);
        r.y.push_back(nums[i + 1]);
        r.vx.push_back(nums[i + 2]);
        r.vy.push_back(nums[i + 3]);
    }

    // Advance 25 steps at a time for part 1 instead of 100 steps to avoid
    // overflowing int16_t.
    r.step(25);
    r.step(25);
    r.step(25);
    r.step(25);
    fmt::print("{}\n", safety_factor(r));

    ThreadPool &pool = ThreadPool::get();
    std::atomic<size_t> min_tree_step = SIZE_MAX;

    // Thread `k` handles [100+k, 100+k+n, 100+k+2n, ...] where `n` is the
    // number of threads.
    pool.for_each_thread([&](size_t thread_id) {
        Robots local_robots = r;

        size_t i = 100 + thread_id;
        local_robots.step(thread_id);

        while (min_tree_step.load(std::memory_order_relaxed) > i) {
            if (has_tree(local_robots)) {
                atomic_store_min(min_tree_step, i, std::memory_order_relaxed);
                return;
            }

            local_robots.step(pool.num_threads());
            i += pool.num_threads();
        }
    });

    fmt::print("{}\n", min_tree_step.load());
}

}
