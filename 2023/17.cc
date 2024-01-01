#include "common.h"

namespace aoc_2023_17 {

enum { N, E, S, W };

struct alignas(4) Vertex {
    Point<uint8_t> p;
    uint8_t direction;
    uint8_t padding;

    uint32_t to_u32() const { return std::bit_cast<uint32_t>(*this); }
    static Vertex from_u32(uint32_t x) { return std::bit_cast<Vertex>(x); }
};
static_assert(sizeof(Vertex) == 4);

static Point<uint8_t> step(Point<uint8_t> p, uint8_t d)
{
    int dx = d == W ? -1 : d == E ? 1 : 0;
    int dy = d == N ? -1 : d == S ? 1 : 0;
    return p.translate(dx, dy);
}

struct Neighbor {
    uint32_t v;
    int cost;
};

static std::span<Neighbor> get_neighbors(std::array<Neighbor, 16> &result,
                                         const Matrix<uint8_t> &m,
                                         uint32_t node,
                                         int min,
                                         int max)
{
    size_t count = 0;
    auto u = Vertex::from_u32(node);

    for (int turn = -1; turn <= 1; turn += 2) {
        uint8_t new_direction = (u.direction + turn) & 3;
        Point<uint8_t> p = u.p;
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
            result[count++] = {Vertex{p, new_direction}.to_u32(), cost};
        }
    }

    return {result.data(), result.data() + count};
}

static int dijkstra(const Matrix<uint8_t> &grid,
                    std::initializer_list<uint32_t> start,
                    std::vector<int> &dist,
                    const int min_straight,
                    const int max_straight,
                    Point<uint8_t> goal)
{
    using VertexPlusDist = std::pair<uint32_t, int>;

    auto proj = [&](const VertexPlusDist &v) { return v.second; };
    DHeap<VertexPlusDist, decltype(proj)> heap(proj);

    std::vector<decltype(heap)::Handle> handles(1 << 20);

    for (uint32_t u : start) {
        dist[u] = 0;
        handles[u] = heap.push({u, 0});
    }

    while (!heap.empty()) {
        auto [u, u_dist] = heap.top();
        heap.pop();

        if (Vertex::from_u32(u).p == goal)
            return u_dist;

        std::array<Neighbor, 16> n;
        for (auto [v, cost] : get_neighbors(n, grid, u, min_straight, max_straight)) {
            const auto new_dist = u_dist + cost;

            if (new_dist >= dist[v])
                continue;

            const auto old_dist = std::exchange(dist[v], new_dist);
            if (old_dist == INT_MAX) {
                handles[v] = heap.push({v, new_dist});
                continue;
            }

            heap.get(handles[v]).second = new_dist;
            heap.decrease_key(handles[v]);
        }
    }

    ASSERT_MSG(false, "Path to {} not found!", goal);
}

void run(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);
    auto grid = Matrix<uint8_t>::from_lines(lines, [&](auto c) { return c - '0'; });
    const Point<uint8_t> goal{
        static_cast<uint8_t>(grid.cols - 1),
        static_cast<uint8_t>(grid.rows - 1),
    };

    std::initializer_list<uint32_t> start = {
        Vertex{{0, 0}, S}.to_u32(),
        Vertex{{0, 0}, E}.to_u32(),
    };
    std::vector<int> dist(1 << 20, INT_MAX);
    fmt::print("{}\n", dijkstra(grid, start, dist, 0, 3, goal));
    std::ranges::fill(dist, INT_MAX);
    fmt::print("{}\n", dijkstra(grid, start, dist, 4, 10, goal));
}

}
