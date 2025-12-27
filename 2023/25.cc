#include "common.h"
#include "dense_map.h"
#include "monotonic_bucket_queue.h"
#include "small_vector.h"
#include "thread_pool.h"
#include <atomic>
#include <random>

namespace aoc_2023_25 {

struct Edge {
    uint16_t to;
    uint16_t weight;
};

struct Graph {
    std::unique_ptr<Edge[]> edge_buffer;
    std::vector<uint16_t> vertex_edge_count;
    std::vector<uint16_t> edge_offsets;
    size_t num_nodes = 0;

    std::span<Edge> edges_of(uint16_t u) const
    {
        return std::span(&edge_buffer[edge_offsets[u] - vertex_edge_count[u]],
                         vertex_edge_count[u]);
    }

    void remove_directed_edge(uint16_t from, uint16_t to)
    {
        auto edges = edges_of(from);
        if (auto it = std::ranges::find_if(edges, λa(a.to == to)); it != edges.end()) {
            std::swap(*it, edges.front());
            vertex_edge_count[from]--;
        }
    }

    size_t size() const { return num_nodes; }
};

static Graph parse_input(std::string_view buf)
{
    auto lines = split_lines(buf);

    struct ParsedLine {
        uint16_t source;
        inplace_vector<uint16_t, 16> targets;
    };

    std::vector<ParsedLine> parsed_lines;
    parsed_lines.reserve(lines.size());

    Graph g;

    {
        std::array<uint16_t, 26 * 26 * 26> table;
        table.fill(UINT16_MAX);

        auto get_node = [&](std::string_view s) -> uint16_t {
            ASSERT(s.size() == 3);
            size_t index = (s[0] - 'a') * 26 * 26 + (s[1] - 'a') * 26 + (s[2] - 'a');
            if (table[index] != UINT16_MAX)
                return table[index];
            return table[index] = g.num_nodes++;
        };

        std::vector<std::string_view> targets;
        targets.reserve(16);
        for (auto &line : lines) {
            auto &parsed = parsed_lines.emplace_back();
            const size_t colon_pos = line.find(':');
            parsed.source = get_node(line.substr(0, colon_pos));
            split(line.substr(colon_pos + 2), targets, ' ');
            for (std::string_view target : targets)
                parsed.targets.push_back(get_node(target));
        }
    }

    g.vertex_edge_count.resize(g.size(), 0);
    g.edge_offsets.resize(g.size(), 0);

    for (auto &[source, targets] : parsed_lines) {
        g.vertex_edge_count[source] += targets.size();
        for (const uint16_t target : targets)
            g.vertex_edge_count[target]++;
    }

    std::exclusive_scan(g.vertex_edge_count.begin(), g.vertex_edge_count.end(),
                        g.edge_offsets.begin(), 0u);
    const size_t n_edges = g.edge_offsets.back() + g.vertex_edge_count.back();
    g.edge_buffer = std::make_unique<Edge[]>(n_edges);

    for (auto &[source, targets] : parsed_lines) {
        for (const uint16_t target : targets) {
            g.edge_buffer[g.edge_offsets[source]++].to = target;
            g.edge_buffer[g.edge_offsets[target]++].to = source;
        }
    }

    return g;
}

struct CrcHasher {
    size_t operator()(uint32_t k) const noexcept { return _mm_crc32_u32(0, k); }
};

static void dijkstra(Graph &g,
                     uint16_t start,
                     uint16_t target,
                     uint16_t *dist,
                     std::pair<uint16_t, uint16_t> *prev,
                     MonotonicBucketQueue<uint16_t> &bq)
{
    bq.clear();
    bq.emplace(0, start);

    std::fill_n(prev, g.size(), std::pair(UINT16_MAX, UINT16_MAX));
    std::fill_n(dist, g.size(), UINT16_MAX - 1);
    dist[start] = 0;

    while (auto u = bq.pop()) {
        if (*u == target)
            break;
        if (dist[*u] != bq.current_priority())
            continue;
        for (Edge &e : g.edges_of(*u)) {
            auto &[v, weight] = e;
            if (auto new_dist = dist[*u] + 1; new_dist < dist[v]) {
                dist[v] = new_dist;
                prev[v] = {*u, &e - g.edge_buffer.get()};
                bq.emplace(new_dist, v);
            }
        }
    }

    auto v = target;
    while (true) {
        auto [u, e] = prev[v];
        if (u == UINT16_MAX)
            break;
        std::atomic_ref(g.edge_buffer[e].weight).fetch_add(1, std::memory_order_relaxed);
        v = u;
    }
}

static int flood(Graph &g, uint32_t start)
{
    std::vector<uint8_t> visited(g.size(), false);
    std::vector<uint32_t> queue;
    queue.reserve(g.size());
    queue.push_back(start);

    while (!queue.empty()) {
        auto u = queue.back();
        queue.pop_back();
        if (!std::exchange(visited[u], true))
            for (auto v : g.edges_of(u))
                queue.push_back(v.to);
    }

    return std::ranges::count(visited, true);
}

void run(std::string_view buf)
{
    ThreadPool &pool = ThreadPool::get();
    auto g = parse_input(buf);

    // The general idea: run Dijkstra's algorithm for a random sample of node
    // pairs in the graph; the edges we are looking for will likely be the most
    // traversed ones, as they are effectively the "bottlenecks" of getting
    // from one component of the graph to the other.
    const size_t n_samples = g.size();
    small_vector<std::pair<uint16_t, uint16_t>, 2048> sample_nodes;
    std::minstd_rand rng(std::random_device{}());
    std::uniform_int_distribution<uint16_t> dist(0, g.size() - 1);
    while (sample_nodes.size() < n_samples) {
        uint16_t u = dist(rng);
        uint16_t v = dist(rng);
        if (u != v)
            sample_nodes.emplace_back(u, v);
    }

    pool.for_each_index(0, sample_nodes.size(), [&](size_t start, size_t end) noexcept {
        MonotonicBucketQueue<uint16_t> bq(2);
        auto dist = std::make_unique_for_overwrite<uint16_t[]>(g.size());
        auto prev =
            std::make_unique_for_overwrite<std::pair<uint16_t, uint16_t>[]>(g.size());
        for (size_t i = start; i < end; ++i) {
            auto [source, target] = sample_nodes[i];
            dijkstra(g, source, target, dist.get(), prev.get(), bq);
        }
    });

    std::atomic_thread_fence(std::memory_order_seq_cst);

    dense_map<uint32_t, int, CrcHasher> edge_counts;
    edge_counts.reserve(g.vertex_edge_count.size());
    for (size_t u = 0; u < g.size(); ++u) {
        for (const auto &[v, weight] : g.edges_of(u)) {
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
        g.remove_directed_edge(u, v);
        g.remove_directed_edge(v, u);
    }

    auto nodes = flood(g, uu);
    fmt::print("{}\n", nodes * (g.size() - nodes));
}

}
