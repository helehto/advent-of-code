#include "common.h"
#include "inplace_vector.h"
#include "monotonic_bucket_queue.h"

namespace aoc_2023_17 {

enum { N, E, S, W };
constexpr uint8_t pad_value = UINT8_MAX;

struct alignas(4) Vertex {
    uint16_t offset;
    uint16_t direction;

    uint32_t to_u32() const { return std::bit_cast<uint32_t>(*this); }
    static Vertex from_u32(uint32_t x) { return std::bit_cast<Vertex>(x); }
};
static_assert(sizeof(Vertex) == 4);

struct Neighbor {
    uint32_t v;
    int cost;
};

template <int MinSteps, int MaxSteps>
[[gnu::noinline]]
static auto
get_neighbors(const uint8_t *grid, const size_t *strides_by_direction, Vertex u)
{
    inplace_vector<Neighbor, 2 * (MaxSteps - MinSteps + 1)> result;

    for (int turn = -1; turn <= 1; turn += 2) {
        const uint8_t new_direction = (u.direction + turn) & 3;
        const size_t stride = strides_by_direction[new_direction];
        const uint8_t *cell = grid + u.offset;
        int cost = 0;
        int n = 1;

        if constexpr (MinSteps > 0) {
            // Check if we can move the minimum number of steps without going
            // out of bounds. This is safe because the grid is padded.
            if (cell[stride * MinSteps] == pad_value)
                continue;

            for (; n < MinSteps; n++) {
                cell += stride;
                cost += *cell;
            }
        }

        for (; n <= MaxSteps; n++) {
            cell += stride;
            if (*cell == pad_value)
                break;
            cost += *cell;
            result.unchecked_emplace_back(
                Vertex{static_cast<uint16_t>(cell - grid), new_direction}.to_u32(), cost);
        }
    }

    return result;
}

template <int MinSteps, int MaxSteps>
static int dijkstra(const uint8_t *grid,
                    size_t cols,
                    std::initializer_list<uint32_t> start,
                    std::vector<uint32_t> &dist,
                    uint16_t goal)
{
    MonotonicBucketQueue<uint32_t> bq(9 * MaxSteps + 1);

    const size_t strides_by_direction[] = {
        /*N*/ -cols,
        /*E*/ 1zu,
        /*S*/ +cols,
        /*W*/ -1zu,
    };

    for (uint32_t u : start) {
        dist[u] = 0;
        bq.emplace(0, u);
    }

    while (auto u32 = bq.pop()) {
        auto u = Vertex::from_u32(*u32);
        if (u.offset == goal) [[unlikely]]
            return bq.current_priority();

        if (dist[*u32] != bq.current_priority())
            continue;

        for (auto [v, cost] :
             get_neighbors<MinSteps, MaxSteps>(grid, strides_by_direction, u)) {
            const auto new_dist = bq.current_priority() + cost;
            if (new_dist < dist[v]) {
                bq.emplace(new_dist, v);
                dist[v] = new_dist;
            }
        }
    }

    ASSERT_MSG(false, "Path not found!");
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<uint8_t>::from_lines(lines, λx(x - '0'));

    // Pad the grid to simplify. This allows us to immediately check if we can
    // move the minimum number of steps without going out of bounds when
    // generating moves for part 2.
    constexpr size_t n_pad = 5;
    grid = grid.padded(n_pad, pad_value);

    // We use 16-bit offsets above to be able to pack a point and direction
    // into 32 bits, so the grid must have fewer than 65536 cells after
    // padding.
    ASSERT(grid.size() < UINT16_MAX);

    const uint16_t goal =
        &grid(grid.rows - n_pad - 1, grid.cols - n_pad - 1) - grid.data();

    std::initializer_list<uint32_t> start = {
        Vertex{static_cast<uint16_t>(n_pad * grid.cols + n_pad), S}.to_u32(),
        Vertex{static_cast<uint16_t>(n_pad * grid.cols + n_pad), E}.to_u32(),
    };
    std::vector<uint32_t> dist(1 << 20, INT_MAX);
    fmt::print("{}\n", dijkstra<0, 3>(grid.data(), grid.cols, start, dist, goal));
    std::ranges::fill(dist, INT_MAX);
    fmt::print("{}\n", dijkstra<4, 10>(grid.data(), grid.cols, start, dist, goal));
}

}
