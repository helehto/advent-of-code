#include "common.h"
#include "dense_map.h"
#include <algorithm>
#include <numeric>
#include <ranges>

namespace aoc_2023_25 {

struct Node {
    int size = 1;
    std::vector<uint16_t> edges;
    std::vector<uint16_t> weights;

    void add_directed_edge(uint16_t v, uint16_t weight)
    {
        for (size_t i = 0; i < edges.size(); i++) {
            if (edges[i] == v) {
                weights[i] += weight;
                return;
            }
        }

        ASSERT(edges.size() == weights.size());
        edges.push_back(v);
        weights.push_back(weight);
    }
};

struct Graph {
    std::vector<Node> nodes;
    dense_map<std::string_view, uint16_t> node_map;
    std::vector<uint16_t> indices;

    void add_edge(uint16_t u, uint16_t v, uint16_t weight)
    {
        nodes[u].add_directed_edge(v, weight);
        nodes[v].add_directed_edge(u, weight);
    }

    void remove_vertex(uint16_t index)
    {
        auto it = std::ranges::find(indices, index);
        ASSERT(it != indices.end());
        erase_swap(indices, it - indices.begin());

        for (auto other_index : nodes[index].edges) {
            auto &other = nodes[other_index];
            for (size_t i = 0; i < other.edges.size(); ++i) {
                if (other.edges[i] == index) {
                    erase_swap(other.edges, i);
                    erase_swap(other.weights, i);
                }
            }
        }
    }

    void merge_vertices(uint16_t u, uint16_t v)
    {
        const auto &t = nodes[v];

        for (size_t i = 0; i < t.edges.size(); i++)
            if (auto w = t.edges[i]; w != u)
                add_edge(u, w, t.weights[i]);

        remove_vertex(v);
    }
};

static Graph parse_input(std::string_view buf)
{
    Graph g;
    auto get_node = [&](std::string_view s) -> uint16_t {
        auto [it, inserted] = g.node_map.try_emplace(s, g.nodes.size());
        if (inserted) {
            g.indices.push_back(g.nodes.size());
            g.nodes.emplace_back();
        }
        return it->second;
    };

    std::vector<std::string_view> fields;
    for (auto &line : split_lines(buf)) {
        split(line, fields, ' ');
        auto source = line.substr(0, line.find(':'));
        for (size_t i = 1; i < fields.size(); ++i)
            g.add_edge(get_node(source), get_node(fields[i]), 1);
    }

    return g;
}

static std::pair<int, int> min_cut_phase(Graph &g, uint16_t a)
{
    std::vector<int> connected_weights(g.nodes.size(), 0);
    connected_weights[a] = INT_MIN / 2;

    BinaryHeap heap(λab(connected_weights[a] > connected_weights[b]), g.nodes.size());

    auto propagate_weights = [&](Node &v) {
        for (size_t i = 0; i < v.edges.size(); i++) {
            connected_weights[v.edges[i]] += v.weights[i];
            heap.decrease(v.edges[i]);
        }
    };
    propagate_weights(g.nodes[a]);

    Node *s = nullptr;
    Node *t = nullptr;
    for (size_t i = 1; i < g.indices.size(); i++) {
        auto index = heap.top();
        heap.pop();
        s = std::exchange(t, &g.nodes[index]);
        propagate_weights(*t);
    }
    ASSERT(s);

    s->size += t->size;
    g.merge_vertices(s - g.nodes.data(), t - g.nodes.data());

    auto cost = std::accumulate(begin(s->weights), end(s->weights), 0);
    return std::pair(s->size, cost);
}

static int min_cut(Graph g)
{
    int cut_size = INT_MAX;
    int min_weight = INT_MAX;
    while (g.indices.size() > 2) {
        auto [size, weight] = min_cut_phase(g, g.indices[0]);
        if (min_weight > weight) {
            min_weight = weight;
            cut_size = size;
        }
    }

    return cut_size;
}

void run(std::string_view buf)
{
    auto g = parse_input(buf);
    int min_cut_size = min_cut(g);
    fmt::print("{}\n", min_cut_size * (g.nodes.size() - min_cut_size));
}

}
