#include "common.h"
#include "dense_map.h"
#include <boost/heap/fibonacci_heap.hpp>

namespace aoc_2023_17 {

enum { N, E, S, W };

struct Vertex {
    Point<uint8_t> p;
    uint8_t direction;

    bool operator==(const Vertex &v) const = default;
};

} // namespace aoc_2023_17

template <>
struct std::hash<aoc_2023_17::Vertex> {
    constexpr size_t operator()(const aoc_2023_17::Vertex &v) const
    {
        size_t h = 0;
        hash_combine(h, v.p, v.direction);
        return h;
    }
};

namespace aoc_2023_17 {

static Point<uint8_t> step(Point<uint8_t> p, uint8_t d, int n)
{
    static int8_t table[] = {0, 1, 0, -1};
    return p.translate(n * table[d], n * table[(d - 1) & 3]);
}

struct Neighbor {
    Vertex v;
    int cost;
};

static boost::container::static_vector<Neighbor, 32>
get_neighbors(const Matrix<uint8_t> &m, Vertex u, int min, int max)
{
    boost::container::static_vector<Neighbor, 32> result;

    for (int turn : {-1, 1}) {
        uint8_t new_direction = (u.direction + turn) & 3;

        int cost = 0;
        for (int n = 1; n <= max; n++) {
            auto next = step(u.p, new_direction, n);
            if (!m.in_bounds(next))
                break;
            cost += m(next);
            if (n >= min)
                result.push_back({{next, new_direction}, cost});
        }
    }

    return result;
}

static int dijkstra(const Matrix<uint8_t> &m,
                    dense_map<Vertex, int> &dist,
                    int min_straight,
                    int max_straight,
                    Point<uint8_t> goal)
{
    auto cmp = [](auto &a, auto &b) { return a.second > b.second; };
    using heap_type = boost::heap::fibonacci_heap<std::pair<Vertex, int>,
                                                  boost::heap::compare<decltype(cmp)>>;
    heap_type heap(cmp);
    dense_map<Vertex, heap_type::handle_type> handles;

    for (auto &u : {Vertex{{0, 0}, S}, Vertex{{0, 0}, E}}) {
        handles.emplace(u, heap.emplace(u, 0));
        dist[u] = 0;
    }

    while (!heap.empty()) {
        auto [u, u_dist] = heap.top();
        heap.pop();
        handles.erase(u);

        if (u.p == goal)
            return u_dist;

        for (const auto &[v, cost] : get_neighbors(m, u, min_straight, max_straight)) {
            const auto new_dist = u_dist + cost;

            auto [it, inserted] = dist.emplace(v, new_dist);
            if (!inserted && it->second <= new_dist)
                continue;
            it->second = new_dist;

            if (auto [it, inserted] = handles.try_emplace(v); !inserted) {
                (*it->second).second = new_dist;
                heap.increase(it->second);
            } else {
                it->second = heap.emplace(v, new_dist);
            }
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

    dense_map<Vertex, int> dist;
    fmt::print("{}\n", dijkstra(grid, dist, 0, 3, goal));
    dist.clear();
    fmt::print("{}\n", dijkstra(grid, dist, 4, 10, goal));
}

}
