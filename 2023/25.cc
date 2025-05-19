#include "common.h"
#include "dense_map.h"
#include "monotonic_bucket_queue.h"
#include "small_vector.h"

namespace aoc_2023_25 {

struct Edge {
    uint16_t to;
    uint16_t weight;
};

struct Node {
    small_vector<Edge, 8> edges;
};

struct Graph {
    std::vector<Node> nodes;
    dense_map<std::string_view, uint16_t> node_map;
    std::vector<uint16_t> indices;
};

static Graph parse_input(std::string_view buf)
{
    auto lines = split_lines(buf);

    Graph g;
    g.nodes.reserve(lines.size());
    g.node_map.reserve(lines.size());
    g.indices.reserve(lines.size());

    auto get_node = [&](std::string_view s) -> uint16_t {
        auto [it, inserted] = g.node_map.try_emplace(s, g.nodes.size());
        if (inserted) {
            g.indices.push_back(g.nodes.size());
            g.nodes.emplace_back();
        }
        return it->second;
    };

    std::vector<std::string_view> fields;
    fields.reserve(16);
    for (auto &line : lines) {
        split(line, fields, ' ');
        auto source = line.substr(0, line.find(':'));
        for (size_t i = 1; i < fields.size(); ++i) {
            const uint16_t u = get_node(source);
            const uint16_t v = get_node(fields[i]);
            g.nodes[u].edges.push_back(Edge{v});
            g.nodes[v].edges.push_back(Edge{u});
        }
    }

    return g;
}

struct crc_hasher {
    size_t operator()(uint32_t k) const noexcept { return _mm_crc32_u32(0, k); }
};

static void dijkstra(Graph &g,
                     uint16_t start,
                     std::vector<uint16_t> &dist,
                     MonotonicBucketQueue<uint16_t> &bq)
{
    bq.clear();
    bq.emplace(0, start);

    dist.assign(g.nodes.size(), UINT16_MAX - 1);
    dist[start] = 0;

    while (auto u = bq.pop()) {
        if (dist[*u] != bq.current_priority())
            continue;
        for (auto &[v, weight] : g.nodes[*u].edges) {
            if (auto new_dist = dist[*u] + 1; new_dist < dist[v]) {
                dist[v] = new_dist;
                bq.emplace(new_dist, v);
                weight++;
            }
        }
    }
}

static int flood(Graph &g, uint32_t start)
{
    std::vector<uint8_t> visited(g.nodes.size(), false);
    std::vector<uint32_t> queue;
    queue.reserve(g.nodes.size());
    queue.push_back(start);

    while (!queue.empty()) {
        auto u = queue.back();
        queue.pop_back();
        if (!std::exchange(visited[u], true))
            for (auto v : g.nodes[u].edges)
                queue.push_back(v.to);
    }

    return std::ranges::count(visited, true);
}

void run(std::string_view buf)
{
    auto g = parse_input(buf);

    int total_edges = std::ranges::fold_left(g.nodes, 0, λab(a + b.edges.size()));
    MonotonicBucketQueue<uint16_t> bq(2);

    // The general idea: run Dijkstra's algorithm for all nodes in the graph;
    // the edges we are looking for will likely be the most traversed ones, as
    // they are effectively the "bottlenecks" of getting from one component of
    // the graph to the other.
    std::vector<uint16_t> dist;
    for (size_t i = 0; i < g.nodes.size(); ++i)
        dijkstra(g, i, dist, bq);

    dense_map<uint32_t, int, crc_hasher> edge_counts;
    edge_counts.reserve(total_edges);
    for (size_t u = 0; u < g.nodes.size(); ++u) {
        for (const auto &[v, weight] : g.nodes[u].edges) {
            const auto a = std::min<uint32_t>(u, v);
            const auto b = std::max<uint32_t>(u, v);
            edge_counts[a << 16 | b] += weight;
        }
    }

    std::vector<std::pair<uint32_t, int>> by_weight(edge_counts.begin(),
                                                    edge_counts.end());
    std::ranges::sort(by_weight, λab(a.second > b.second));

    uint16_t uu = by_weight[0].first >> 16;
    for (size_t i = 0; i < 3; ++i) {
        uint16_t u = by_weight[i].first >> 16;
        uint16_t v = by_weight[i].first & 0xffff;
        erase_if(g.nodes[u].edges, λa(a.to == v));
        erase_if(g.nodes[v].edges, λa(a.to == u));
    }

    auto nodes = flood(g, uu);
    fmt::print("{}\n", nodes * (g.nodes.size() - nodes));
}

}
