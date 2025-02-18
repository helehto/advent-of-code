#include "common.h"
#include "dense_set.h"
#include <algorithm>
#include <vector>

namespace aoc_2023_23 {

struct alignas(16) Node {
    uint8_t n_edges = 0;
    uint8_t edges[4];
    uint16_t weights[4];
};

struct Graph {
    std::vector<Node> nodes;
    dense_map<Vec2z, size_t> node_map;
    uint8_t start_index;
    uint8_t goal_index;
};

static constexpr uint64_t bit(int i)
{
    return UINT64_C(1) << i;
}

template <bool IncludePredecessors>
__attribute__((noinline)) static int64_t search(const Graph &graph)
{
    struct State {
        uint64_t visited_mask;
        const Node *node;
        uint16_t total_weight;
    };

    auto stack = std::make_unique<State[]>(16384);
    State *p = stack.get();

    *p++ = State{
        .visited_mask = 0,
        .node = &graph.nodes[graph.start_index],
        .total_weight = 0,
    };

    uint16_t max_length = 0;

    while (p != stack.get()) {
        auto [visited_mask, node, total_weight] = *--p;
        const auto this_mask = bit(node - graph.nodes.data());

        if (node == &graph.nodes[graph.goal_index]) {
            max_length = std::max(max_length, total_weight);
            continue;
        }

        for (size_t i = 0; i < node->n_edges; ++i) {
            const auto next = node->edges[i] & ~0x80;
            const auto weight = node->weights[i];

            if constexpr (!IncludePredecessors) {
                if (node->edges[i] & 0x80)
                    continue;
            }

            if ((visited_mask & bit(next)) == 0) {
                *p++ = State{
                    .visited_mask = visited_mask | this_mask,
                    .node = &graph.nodes[next],
                    .total_weight = static_cast<uint16_t>(total_weight + weight),
                };
            }
        }
    }

    return max_length;
}

static size_t get_node(Graph &graph, Vec2z p)
{
    if (auto it = graph.node_map.find(p); it != graph.node_map.end()) {
        return it->second;
    } else {
        graph.nodes.emplace_back();
        graph.node_map.emplace(p, graph.nodes.size() - 1);
        return graph.nodes.size() - 1;
    }
}

static void add_edge(Graph &graph, Vec2z p, Vec2z q, int weight)
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

    const auto pn = get_node(graph, p);
    const auto qn = get_node(graph, q);
    add_unique(graph.nodes[pn], qn);
    add_unique(graph.nodes[qn], pn | 0x80);
}

static Graph build_graph(const Matrix<char> &grid, const Vec2z start, const Vec2z goal)
{
    Graph graph;

    std::vector<std::pair<Vec2z, Vec2z>> queue{{start, start}};
    std::vector<Vec2z> neighbors;

    for (size_t i = 0; i < queue.size(); i++) {
        auto [origin, curr] = queue[i];
        auto prev = origin;

        for (int weight = 1;; weight++) {
            if (curr == goal) {
                add_edge(graph, origin, curr, weight);
                break;
            }

            // Generate candidate moves to determine if this is a crossing,
            // regardless of icy slopes.
            neighbors.clear();
            for (auto q : neighbors4(grid, curr))
                if (q != prev && grid(q) != '#')
                    neighbors.push_back(q);

            if (neighbors.size() != 1) {
                // It's a crossing; filter by icy slopes to get the next possible
                // moves from here.
                erase_if(neighbors, [&](const Vec2z &next) {
                    char c = grid(next);
                    int dx = next.x - curr.x;
                    int dy = next.y - curr.y;
                    return c != '.' && !(c == '<' && dx < 0) && !(c == '>' && dx > 0) &&
                           !(c == '^' && dy < 0) && !(c == 'v' && dy > 0);
                });
                ASSERT(!neighbors.empty());

                add_edge(graph, origin, curr, weight);
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
    ASSERT(graph.nodes.size() < 64);

    fmt::print("{}\n", search<false>(graph) - 1);
    fmt::print("{}\n", search<true>(graph) - 1);
}

}
