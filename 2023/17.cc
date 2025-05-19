#include "common.h"
#include "inplace_vector.h"
#include "monotonic_bucket_queue.h"

namespace aoc_2023_17 {

enum { N, E, S, W };

struct alignas(4) Vertex {
    Vec2u8 p;
    uint8_t direction;
    uint8_t padding;

    uint32_t to_u32() const { return std::bit_cast<uint32_t>(*this); }
    static Vertex from_u32(uint32_t x) { return std::bit_cast<Vertex>(x); }
};
static_assert(sizeof(Vertex) == 4);

static Vec2u8 step(Vec2u8 p, uint8_t d)
{
    int dx = d == W ? -1 : d == E ? 1 : 0;
    int dy = d == N ? -1 : d == S ? 1 : 0;
    return p + Vec2u8(dx, dy);
}

struct Neighbor {
    uint32_t v;
    int cost;
};

static inplace_vector<Neighbor, 16>
get_neighbors(const Matrix<uint8_t> &m, Vertex u, int min, int max)
{
    inplace_vector<Neighbor, 16> result;

    for (int turn = -1; turn <= 1; turn += 2) {
        uint8_t new_direction = (u.direction + turn) & 3;
        Vec2u8 p = u.p;
        int cost = 0;

        int n = 1;
        for (n = 1; n < min; n++) {
            p = step(p, new_direction);
            if (!m.in_bounds(p))
                break;
            cost += m(p);
        }

        for (; n <= max; n++) {
            p = step(p, new_direction);
            if (!m.in_bounds(p))
                break;
            cost += m(p);
            result.unchecked_push_back(Neighbor{Vertex{p, new_direction}.to_u32(), cost});
        }
    }

    return result;
}

static int dijkstra(const Matrix<uint8_t> &grid,
                    std::initializer_list<uint32_t> start,
                    std::vector<uint32_t> &dist,
                    const int min_straight,
                    const int max_straight,
                    Vec2u8 goal)
{
    MonotonicBucketQueue<uint32_t> bq(9 * max_straight + 1);

    for (uint32_t u : start) {
        dist[u] = 0;
        bq.emplace(0, u);
    }

    while (auto u32 = bq.pop()) {
        auto u = Vertex::from_u32(*u32);
        if (u.p == goal)
            return bq.current_priority();

        if (dist[*u32] != bq.current_priority())
            continue;

        for (auto [v, cost] : get_neighbors(grid, u, min_straight, max_straight)) {
            const auto new_dist = bq.current_priority() + cost;
            if (new_dist < dist[v]) {
                bq.emplace(new_dist, v);
                dist[v] = new_dist;
            }
        }
    }

    ASSERT_MSG(false, "Path to {} not found!", goal);
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<uint8_t>::from_lines(lines, λx(x - '0'));
    const Vec2u8 goal{
        static_cast<uint8_t>(grid.cols - 1),
        static_cast<uint8_t>(grid.rows - 1),
    };

    std::initializer_list<uint32_t> start = {
        Vertex{{0, 0}, S}.to_u32(),
        Vertex{{0, 0}, E}.to_u32(),
    };
    std::vector<uint32_t> dist(1 << 20, INT_MAX);
    fmt::print("{}\n", dijkstra(grid, start, dist, 0, 3, goal));
    std::ranges::fill(dist, INT_MAX);
    fmt::print("{}\n", dijkstra(grid, start, dist, 4, 10, goal));
}

}
