#include "common.h"
#include "dense_map.h"
#include <boost/heap/fibonacci_heap.hpp>

enum { N, E, S, W };

struct Vertex {
    Point<uint8_t> p;
    uint8_t direction;
    uint8_t n_forward;

    bool operator==(const Vertex &v) const = default;
};

template <>
struct std::hash<Vertex> {
    constexpr size_t operator()(const Vertex &v) const
    {
        size_t h = 0;
        hash_combine(h, v.p, v.direction, v.n_forward);
        return h;
    }
};

static Point<uint8_t> step(Point<uint8_t> p, uint8_t d)
{
    static int8_t table[] = {0, 1, 0, -1};
    return p.translate(table[d], table[(d - 1) & 3]);
}

template <typename Map, typename Key>
static auto
get_or(const Map &m,
       const Key &key,
       decltype(std::declval<typename Map::value_type>().second) default_value)
{
    auto it = m.find(key);
    return it != m.end() ? it->second : default_value;
}

static int dijkstra(const Matrix<uint8_t> &m,
                    dense_map<Vertex, int> &dist,
                    int min_straight,
                    int max_straight,
                    Point<uint8_t> goal)
{
    std::vector<Vertex> queue{
        Vertex{{0, 0}, S, 0},
        Vertex{{0, 0}, E, 0},
    };

    auto cmp = [](auto &a, auto &b) { return a.second > b.second; };

    using heap_type = boost::heap::fibonacci_heap<std::pair<Vertex, int>,
                                                  boost::heap::compare<decltype(cmp)>>;
    heap_type heap(cmp);
    dense_map<Vertex, heap_type::handle_type> handles;

    for (auto &u : queue) {
        handles.emplace(u, heap.emplace(u, 0));
        dist[u] = 0;
    }

    while (!heap.empty()) {
        auto [u, u_dist] = heap.top();
        heap.pop();
        auto [p, direction, u_straight] = u;
        handles.erase(u);

        if (p == goal)
            return u_dist;

        boost::container::static_vector<Vertex, 3> neighbors;

        auto addn = [&](int d, int n) {
            auto next = step(p, d);
            if (m.in_bounds(next) && (next != goal || n >= min_straight))
                neighbors.emplace_back(next, d, n);
        };

        if (u_straight < max_straight)
            addn(direction, u_straight + 1);
        if (u_straight >= min_straight) {
            addn((direction + 1) & 3, 1);
            addn((direction - 1) & 3, 1);
        }

        for (const auto &v : neighbors) {
            const auto q = v.p;
            const auto new_dist = u_dist + m(q);

            auto [it, inserted] = dist.emplace(v, new_dist);
            if (!inserted && new_dist >= it->second)
                continue;

            if (auto [it, inserted] = handles.try_emplace(v); !inserted)
                heap.increase(it->second);
            else
                it->second = heap.emplace(v, new_dist);
        }
    }

    ASSERT_MSG(false, "Path to {} not found!", goal);
}

void run_2023_17(FILE *f)
{
    auto grid = Matrix<uint8_t>::from_lines(getlines(f), [&](auto c) { return c - '0'; });
    const Point<uint8_t> goal{
        static_cast<uint8_t>(grid.cols - 1),
        static_cast<uint8_t>(grid.rows - 1),
    };

    dense_map<Vertex, int> dist;
    fmt::print("{}\n", dijkstra(grid, dist, 0, 3, goal));
    dist.clear();
    fmt::print("{}\n", dijkstra(grid, dist, 4, 10, goal));
}
