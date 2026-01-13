#include "common.h"
#include "dense_map.h"
#include "monotonic_bucket_queue.h"

namespace aoc_2023_21 {

struct alignas(4) WrappedPosition {
    uint16_t offset;
    int8_t chunk_x;
    int8_t chunk_y;

    bool operator==(const WrappedPosition &other) const noexcept
    {
        return std::bit_cast<uint32_t>(*this) == std::bit_cast<uint32_t>(other);
    }
};

static void walk(MatrixView<const char> grid, Vec2i start, size_t max_steps, auto &&sink)
{
    const char *data = grid.data();
    dense_map<WrappedPosition, int, CrcHasher> dist;
    dist.reserve(10 * grid.size());
    WrappedPosition wrapped_start(grid.cols * start.y + start.x, 0, 0);
    dist.emplace(wrapped_start, 0);

    sink(0);

    MonotonicBucketQueue<WrappedPosition, small_vector<WrappedPosition, 128>> queue(2);
    queue.emplace(0, wrapped_start);

    const ssize_t xwrap = static_cast<ssize_t>(grid.cols - 2);
    const ssize_t ywrap = static_cast<ssize_t>(grid.size() - 2 * grid.cols);

    while (auto p = queue.pop()) {
        const auto d = queue.current_priority();

        if (d >= max_steps) [[unlikely]]
            continue;

        auto make_wrapped_position_x = [&](ssize_t stride, ssize_t sign) {
            auto new_offset = p->offset + stride;
            auto mask = static_cast<ssize_t>(static_cast<int8_t>(data[new_offset])) >> 63;
            return WrappedPosition(new_offset + sign * (xwrap & mask),
                                   p->chunk_x + sign * mask, p->chunk_y);
        };

        auto make_wrapped_position_y = [&](ssize_t stride, ssize_t sign) {
            auto new_offset = p->offset + stride;
            auto mask = static_cast<ssize_t>(static_cast<int8_t>(data[new_offset])) >> 63;
            return WrappedPosition(new_offset + sign * (ywrap & mask), p->chunk_x,
                                   p->chunk_y + sign * mask);
        };

        const WrappedPosition neighbors[] = {
            make_wrapped_position_x(-1, 1),
            make_wrapped_position_x(1, -1),
            make_wrapped_position_y(-grid.cols, 1),
            make_wrapped_position_y(grid.cols, -1),
        };
        for (const WrappedPosition &q : neighbors) {
            if (data[q.offset] != '#' && dist.emplace(q, d + 1).second) {
                sink(d + 1);
                queue.emplace(d + 1, q);
            }
        }
    }
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines).padded(1, (char)-1);

    Vec2i start;
    for (Vec2z p : grid.ndindex())
        if (grid(p) == 'S')
            start = p.cast<int>();

    ASSERT(grid.rows == grid.cols);
    const int64_t N = grid.rows - 2;

    int64_t part1 = 0;
    int64_t y0 = 0;
    int64_t s0 = N / 2 + 0 * N;
    int64_t y1 = 0;
    int64_t s1 = N / 2 + 1 * N;
    int64_t y2 = 0;
    int64_t s2 = N / 2 + 2 * N;
    walk(grid, start, N / 2 + 2 * N, [&](int d) {
        part1 += (d % 2 == 0 && d <= 64);
        y0 += (d % 2 == (s0 & 1) && d <= s0);
        y1 += (d % 2 == (s1 & 1) && d <= s1);
        y2 += (d % 2 == (s2 & 1) && d <= s2);
    });

    int64_t steps = 26501365;
    ASSERT(26501365 % N == N / 2);

    // Lagrange interpolation for x = 0,1,2:
    auto P = [&](double x) -> double {
        return y0 * (x - 1) * (x - 2) / ((0 - 1) * (0 - 2)) +
               y1 * (x - 0) * (x - 2) / ((1 - 0) * (1 - 2)) +
               y2 * (x - 0) * (x - 1) / ((2 - 0) * (2 - 1));
    };
    fmt::print("{}\n", part1);
    fmt::print("{}\n", P((steps - N / 2) / N));
}

}
