#include <algorithm>
#include <aoc/base.h>
#include <aoc/macros.h>
#include <aoc/math.h>
#include <aoc/string.h>
#include <aoc/thread_pool.h>
#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace aoc_2015_6 {

struct Operation {
    enum class Type : uint8_t { toggle = ' ', turn_on = 'n', turn_off = 'f' };
    Type type;
    int16_t x0, y0, x1, y1;
};

constexpr void sort_unique_coordinates(std::vector<int16_t> &xs)
{
    std::array<uint64_t, (1001 + 63) / 64> seen{};
    for (int16_t x : xs)
        seen[x / 64] |= UINT64_C(1) << (x % 64);

    size_t k = 0;
    for (size_t i = 0; i < seen.size(); ++i)
        for (auto m = seen[i]; m; m &= m - 1)
            xs[k++] = i * 64 + std::countr_zero(m);

    xs.resize(k);
}

struct CompressedInput {
    std::vector<int16_t> xs;
    std::vector<int16_t> ys;
    std::vector<Operation> ops;
};

constexpr CompressedInput parse_input(std::string_view buf)
{
    const auto lines = split_lines(buf);

    std::vector<Operation> ops(lines.size());
    std::vector<int16_t> xs(2 * lines.size());
    std::vector<int16_t> ys(2 * lines.size());

    for (size_t i = 0; std::string_view line : lines) {
        auto [x0, y0, x1, y1] = find_numbers_n<int16_t, 4>(line);
        x1++, y1++; // inclusive -> exclusive
        ASSERT_MSG(0 <= x0 && x0 <= x1, "bad line: {}", line);
        ASSERT_MSG(0 <= y0 && y0 <= y1, "bad line: {}", line);

        const char c = line[6];
        ASSERT_MSG(c == ' ' || c == 'f' || c == 'n', "bad line: {}", line);
        xs[2 * i + 0] = x0;
        xs[2 * i + 1] = x1;
        ys[2 * i + 0] = y0;
        ys[2 * i + 1] = y1;
        ops[i++] = Operation{static_cast<Operation::Type>(c), x0, y0, x1, y1};
    }

    // Compress to reduce the effective size of the grid.
    sort_unique_coordinates(xs);
    sort_unique_coordinates(ys);
    for (auto &[c, x0, y0, x1, y1] : ops) {
        x0 = std::ranges::lower_bound(xs, x0) - xs.begin();
        x1 = std::ranges::lower_bound(xs, x1) - xs.begin();
        y0 = std::ranges::lower_bound(ys, y0) - ys.begin();
        y1 = std::ranges::lower_bound(ys, y1) - ys.begin();
    }

    return {std::move(xs), std::move(ys), std::move(ops)};
}

constexpr std::pair<int, int>
handle_slice(const CompressedInput &input, const ssize_t full_y0, const ssize_t full_y1)
{
    auto &[xs, ys, ops] = input;
    auto rows = full_y1 - full_y0;

    // Bit 15 (0x8000) signifies whether the light is on, and the lower 15 bits
    // signifies the brightness; this allows for solving both parts in a single
    // pass over the operations.
    Matrix<uint16_t> state(rows, xs.size() - 1);

    for (auto [c, x0, y0, x1, y1] : ops) {
        y0 = std::max<ssize_t>(y0 - full_y0, 0);
        y1 = std::min<ssize_t>(y1 - full_y0, rows);

        if (c == Operation::Type::toggle) {
            for (ssize_t y = y0; y < y1; y++)
                for (auto &x : std::span(&state(y, x0), x1 - x0))
                    x = (x + 2) ^ 0x8000;
        } else if (c == Operation::Type::turn_off) {
            for (ssize_t y = y0; y < y1; y++) {
                for (auto &x : std::span(&state(y, x0), x1 - x0)) {
                    const auto z = x & 0x7fff;
                    x = z > 0 ? z - 1 : 0;
                }
            }
        } else {
            for (ssize_t y = y0; y < y1; y++)
                for (auto &x : std::span(&state(y, x0), x1 - x0))
                    x = (x + 1) | 0x8000;
        }
    }

    int n_lights = 0;
    int total_brightness = 0;
    for (size_t y = 0; y < state.rows; y++) {
        int b = 0;
        int n = 0;
        const auto dy = ys[y + full_y0 + 1] - ys[y + full_y0];
        for (size_t x = 0; x < state.cols; x++) {
            const auto dx = xs[x + 1] - xs[x];
            b += (state(y, x) & ~0x8000) * dx;
            n += (state(y, x) & 0x8000) ? dx : 0;
        }
        n_lights += n * dy;
        total_brightness += b * dy;
    }

    return {n_lights, total_brightness};
}

void run(std::string_view buf, aoc::Answer &answer)
{
    const auto input = parse_input(buf);
    int n_lights = 0;
    int total_brightness = 0;

    const auto max_x = std::ranges::max(input.ops, {}, λa(a.x1)).x1;
    const auto max_y = std::ranges::max(input.ops, {}, λa(a.y1)).y1;
    ASSERT(max_x <= 1000);
    ASSERT(max_y <= 1000);

    // All operations are element-wise operations on the grid, so this is
    // embarrassingly parallel. Slice the grid into strips of contiguous rows
    // and hand them off to different threads.
    ThreadPool::get().for_each_index(0, max_y, [&](size_t y0, size_t y1) {
        auto [nl, tb] = handle_slice(input, y0, y1);
        std::atomic_ref(n_lights).fetch_add(nl, std::memory_order_relaxed);
        std::atomic_ref(total_brightness).fetch_add(tb, std::memory_order_relaxed);
    });

    answer.add(n_lights);
    answer.add(total_brightness);
}
AOC_REGISTER_SOLVER(2015, 6, run);

}
