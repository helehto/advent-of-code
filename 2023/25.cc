#include "common.h"
#include "dense_map.h"
#include "monotonic_bucket_queue.h"
#include "small_vector.h"
#include "thread_pool.h"
#include <atomic>

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

struct CrcHasher {
    size_t operator()(uint32_t k) const noexcept { return _mm_crc32_u32(0, k); }
};

static void
dijkstra(Graph &g, uint16_t start, uint16_t *dist, MonotonicBucketQueue<uint16_t> &bq)
{
    bq.clear();
    bq.emplace(0, start);

    std::fill_n(dist, g.nodes.size(), UINT16_MAX - 1);
    dist[start] = 0;

    while (auto u = bq.pop()) {
        if (dist[*u] != bq.current_priority())
            continue;
        for (auto &[v, weight] : g.nodes[*u].edges) {
            if (auto new_dist = dist[*u] + 1; new_dist < dist[v]) {
                dist[v] = new_dist;
                bq.emplace(new_dist, v);
                std::atomic_ref(weight).fetch_add(1, std::memory_order_relaxed);
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
    ThreadPool &pool = ThreadPool::get();
    auto g = parse_input(buf);

    int total_edges = std::ranges::fold_left(g.nodes, 0, λab(a + b.edges.size()));

    // The general idea: run Dijkstra's algorithm for all nodes in the graph;
    // the edges we are looking for will likely be the most traversed ones, as
    // they are effectively the "bottlenecks" of getting from one component of
    // the graph to the other.
    pool.for_each_thread([&](size_t thread_id) {
        MonotonicBucketQueue<uint16_t> bq(2);
        auto dist = std::make_unique_for_overwrite<uint16_t[]>(g.nodes.size());
        for (size_t i = thread_id; i < g.nodes.size(); i += pool.num_threads())
            dijkstra(g, i, dist.get(), bq);
    });

    std::atomic_thread_fence(std::memory_order_seq_cst);

    dense_map<uint32_t, int, CrcHasher> edge_counts;
    edge_counts.reserve(total_edges);
    for (size_t u = 0; u < g.nodes.size(); ++u) {
        for (const auto &[v, weight] : g.nodes[u].edges) {
            const auto a = std::min<uint32_t>(u, v);
            const auto b = std::max<uint32_t>(u, v);
            edge_counts[a << 16 | b] +=
                std::atomic_ref(weight).load(std::memory_order_relaxed);
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
