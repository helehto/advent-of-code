#include "common.h"
#include "dense_map.h"
#include "thread_pool.h"
#include <atomic>
#include <latch>
#include <random>

namespace aoc_2023_23 {

struct alignas(16) Node {
    uint8_t n_edges = 0;
    int8_t distance_from_goal = -1;
    uint8_t edges[4];
    uint8_t padding[2];
    uint16_t weights[4];
};
static_assert(sizeof(Node) == 16);

struct Graph {
    inplace_vector<Node, 64> nodes;
    dense_map<Vec2z, size_t> node_map;
    uint8_t start_index;
    uint8_t goal_index;

    size_t get_node(Vec2z p)
    {
        if (auto it = node_map.find(p); it != node_map.end()) {
            return it->second;
        } else {
            nodes.emplace_back();
            node_map.emplace(p, nodes.size() - 1);
            return nodes.size() - 1;
        }
    }

    void add_edge(Vec2z p, Vec2z q, int weight)
    {
        auto add_unique = [&](Node &node, uint8_t v) {
            for (size_t i = 0; i < node.n_edges; i++) {
                if (node.edges[i] == v)
                    return;
            }
            node.edges[node.n_edges] = v;
            node.weights[node.n_edges] = weight;
            node.n_edges++;
        };

        const auto pn = get_node(p);
        const auto qn = get_node(q);
        add_unique(nodes[pn], qn);
        add_unique(nodes[qn], pn | 0x80);
    }
};

struct alignas(16) State {
    uint64_t visited_mask;
    uint8_t node_index;
    uint16_t total_weight;
    bool ignore_slopes;
};
static_assert(sizeof(State) == 16);

static Graph build_graph(MatrixView<const char> grid, const Vec2z start, const Vec2z goal)
{
    Graph graph;

    small_vector<std::pair<Vec2z, Vec2z>, 128> queue{{start, start}};

    for (size_t i = 0; i < queue.size(); i++) {
        auto [origin, curr] = queue[i];
        auto prev = origin;

        for (int weight = 1;; weight++) {
            if (curr == goal) {
                graph.add_edge(origin, curr, weight);
                break;
            }

            // Generate candidate moves to determine if this is a crossing,
            // regardless of icy slopes.
            inplace_vector<Vec2z, 4> neighbors;
            for (auto q : neighbors4(grid, curr))
                if (q != prev && grid(q) != '#')
                    neighbors.unchecked_push_back(q);

            if (neighbors.size() != 1) {
                // It's a crossing; filter by icy slopes to get the next possible
                // moves from here.
                erase_if(neighbors, [&](const Vec2z &next) noexcept {
                    char c = grid(next);
                    int dx = next.x - curr.x;
                    int dy = next.y - curr.y;
                    return c != '.' && !(c == '<' && dx < 0) && !(c == '>' && dx > 0) &&
                           !(c == '^' && dy < 0) && !(c == 'v' && dy > 0);
                });
                ASSERT(!neighbors.empty());

                graph.add_edge(origin, curr, weight);
                for (size_t j = 0; j < neighbors.size(); j++)
                    queue.emplace_back(curr, neighbors[j]);
                break;
            }

            // It's a corridor, keep walking along it.
            prev = std::exchange(curr, neighbors.front());
        }
    }

    graph.start_index = graph.node_map.at(start);
    graph.goal_index = graph.node_map.at(goal);

    // BFS from goal to find the shortest distance to each node in terms of
    // number of intersections.
    {
        graph.nodes[graph.goal_index].distance_from_goal = 0;
        inplace_vector<std::pair<Node *, size_t>, 128> queue{
            {&graph.nodes[graph.goal_index], 0}};
        for (size_t i = 0; i < queue.size(); ++i) {
            const auto [u, d] = queue[i];
            for (size_t j = 0; j < u->n_edges; ++j) {
                const auto v = &graph.nodes[u->edges[j] & ~0x80];
                if (v->distance_from_goal < 0) {
                    queue.emplace_back(v, d + 1);
                    v->distance_from_goal = d + 1;
                }
            }
        }
    }

    return graph;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines);

    Vec2z start;
    for (size_t j = 0; j < grid.cols; j++) {
        start = {j, 0};
        if (std::exchange(grid(start), '#') == '.')
            break;
    }

    Vec2z goal;
    for (size_t j = 0; j < grid.cols; j++) {
        goal = {j, grid.rows - 1};
        if (grid(goal) == '.')
            break;
    }

    auto graph = build_graph(grid, start, goal);

    std::array<std::atomic<size_t>, 2> solutions{};
    {
        ThreadPool &pool = ThreadPool::get();
        ForkPool<State> fork_pool(pool.num_threads());

        fork_pool.push({
            State{.node_index = graph.start_index, .ignore_slopes = false},
            State{.node_index = graph.start_index, .ignore_slopes = true},
        });

        fork_pool.run(pool, [&](const State &u, small_vector_base<State> &next_states) {
            auto [visited_mask, node_index, total_weight, ignore_slopes] = u;
            if (node_index == graph.goal_index) {
                auto &sol = ignore_slopes ? solutions[1] : solutions[0];
                atomic_store_max(sol, static_cast<size_t>(total_weight));
                return;
            }

            const Node &node = graph.nodes[node_index];
            for (size_t i = 0; i < node.n_edges; ++i) {
                if (!ignore_slopes && (node.edges[i] & 0x80) != 0)
                    continue;

                const uint8_t next_index = node.edges[i] & ~0x80;
                const Node &next = graph.nodes[next_index];

                // Avoid dead ends: the graph is a variant of a rook's graph
                // (https://en.wikipedia.org/wiki/Rook%27s_graph), but where only
                // adjacent nodes on the board are connected with an edge.
                if (node.n_edges == 3 && next.n_edges == 3 &&
                    next.distance_from_goal > node.distance_from_goal)
                    continue;

                if ((visited_mask & (UINT64_C(1) << next_index)) == 0)
                    next_states.push_back(State{
                        .visited_mask = visited_mask | (UINT64_C(1) << node_index),
                        .node_index = next_index,
                        .total_weight =
                            static_cast<uint16_t>(total_weight + node.weights[i]),
                        .ignore_slopes = ignore_slopes,
                    });
            }
        });
    }

    fmt::print("{}\n{}\n", solutions[0].load() - 1, solutions[1].load() - 1);
}

}
