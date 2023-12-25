#include "common.h"
#include "dense_set.h"
#include <algorithm>

namespace aoc_2023_23 {

struct Node {
    std::vector<std::pair<uint8_t, uint16_t>> pred;
    std::vector<std::pair<uint8_t, uint16_t>> succ;
};

struct Graph {
    std::vector<Node> nodes;
    dense_map<Point<size_t>, size_t> node_map;
    size_t start_index;
    size_t goal_index;
};

template <bool IncludePredecessors>
static int64_t search(const Graph &graph,
                      const uint64_t visited_mask,
                      const size_t curr_index,
                      const int64_t curr_length)
{
    if (curr_index == graph.goal_index)
        return curr_length;

    int64_t max_length = 0;

    if constexpr (IncludePredecessors) {
        for (auto &[next, weight] : graph.nodes[curr_index].pred) {
            auto bit = UINT64_C(1) << next;
            if ((visited_mask & bit) == 0) {
                auto r = search<IncludePredecessors>(graph, visited_mask | bit, next,
                                                     curr_length + weight);
                max_length = std::max(max_length, r);
            }
        }
    }

    for (auto &[next, weight] : graph.nodes[curr_index].succ) {
        auto bit = UINT64_C(1) << next;
        if ((visited_mask & bit) == 0) {
            auto r = search<IncludePredecessors>(graph, visited_mask | bit, next,
                                                 curr_length + weight);
            max_length = std::max(max_length, r);
        }
    }

    return max_length;
}

static size_t get_node(Graph &graph, Point<size_t> p)
{
    if (auto it = graph.node_map.find(p); it != graph.node_map.end()) {
        return it->second;
    } else {
        graph.nodes.emplace_back();
        graph.node_map.emplace(p, graph.nodes.size() - 1);
        return graph.nodes.size() - 1;
    }
}

static void add_edge(Graph &graph, Point<size_t> p, Point<size_t> q, int weight)
{
    auto add_unique = [&](auto &edges, size_t v) {
        for (auto &[w, _] : edges)
            if (v == w)
                return;
        edges.emplace_back(v, weight);
    };

    const auto pn = get_node(graph, p);
    const auto qn = get_node(graph, q);
    add_unique(graph.nodes[pn].succ, qn);
    add_unique(graph.nodes[qn].pred, pn);
}

static Graph
build_graph(const Matrix<char> &grid, const Point<size_t> start, const Point<size_t> goal)
{
    Graph graph;

    std::vector<std::pair<Point<size_t>, Point<size_t>>> queue{{start, start}};
    std::vector<Point<size_t>> neighbors;

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
                erase_if(neighbors, [&](const Point<size_t> &next) {
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

void run(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);
    auto grid = Matrix<char>::from_lines(lines);

    Point<size_t> start;
    for (size_t j = 0; j < grid.cols; j++) {
        start = {j, 0};
        if (std::exchange(grid(start), '#') == '.')
            break;
    }

    Point<size_t> goal;
    for (size_t j = 0; j < grid.cols; j++) {
        goal = {j, grid.rows - 1};
        if (grid(goal) == '.')
            break;
    }

    auto graph = build_graph(grid, start, goal);
    ASSERT(graph.nodes.size() < 64);

    fmt::print("{}\n", search<false>(graph, 0, graph.start_index, 0) - 1);
    fmt::print("{}\n", search<true>(graph, 0, graph.start_index, 0) - 1);
}

}
