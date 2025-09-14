#include "common.h"
#include "dense_map.h"
#include "thread_pool.h"
#include <atomic>
#include <random>

namespace aoc_2023_23 {

struct alignas(16) Node {
    uint8_t n_edges = 0;
    int8_t distance_from_goal = -1;
    uint8_t edges[4];
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

// Implementation of a concurrent lock-free deque.
//
// See "Dynamic Circular Work-Stealing Deque" by Chase and Lev (2005) for the
// initial paper describing this algorithm. This implementation follows the
// pseudocode in "Correct and Efficient Work-Stealing for Weak Memory Models"
// by Lê et al (2013).
struct WorkQueue {
    static constexpr size_t buffer_size = 64;
    static constexpr size_t mask = buffer_size - 1;

    // Accessed by owner threads (push/pop) *and* thieves (steal):
    alignas(64) std::array<State, buffer_size> buffer;

    // Accessed by owner threads:
    alignas(64) std::atomic<uint64_t> bottom = 0;

    // Accessed by thieves:
    alignas(64) std::atomic<uint64_t> top = 0;

    bool pop(State &result)
    {
        const uint64_t b = bottom.load(std::memory_order_relaxed) - 1;
        bottom.store(b, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);

        uint64_t t = top.load(std::memory_order_relaxed);
        if (static_cast<int64_t>(t) <= static_cast<int64_t>(b)) {
            result = buffer[b & mask];
            if (t == b) {
                // Single last element in queue.
                const bool ok = top.compare_exchange_strong(
                    t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed);
                bottom.store(b + 1, std::memory_order_relaxed);
                return ok;
            }
        } else {
            // Empty queue.
            bottom.store(b + 1, std::memory_order_relaxed);
            return false;
        }

        return true;
    }

    void push(const State &state)
    {
        const uint64_t b = bottom.load(std::memory_order_relaxed);
        const uint64_t t = top.load(std::memory_order_acquire);
        ASSERT(static_cast<int64_t>(b) - static_cast<int64_t>(t) <
               static_cast<int64_t>(buffer_size));
        buffer[b & mask] = state;
        std::atomic_thread_fence(std::memory_order_release);
        bottom.store(b + 1, std::memory_order_relaxed);
    }

    bool steal(State &result)
    {
        uint64_t t = top.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        const uint64_t b = bottom.load(std::memory_order_acquire);
        if (t >= b)
            return false;

        result = buffer[t & mask];
        return top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst,
                                           std::memory_order_relaxed);
    }
};

static void search(size_t thread_id,
                   size_t n_threads,
                   const Graph &graph,
                   std::atomic<size_t> &n_thieves,
                   std::atomic<size_t> *solutions,
                   WorkQueue *all_queues)
{
    WorkQueue &queue = all_queues[thread_id];
    State u;

    small_vector<size_t, 32> victim_order;
    for (size_t i = 0; i < n_threads; ++i)
        if (i != thread_id)
            victim_order.push_back(i);

    // Randomize the order in which each thief tries to steal work from the
    // other threads, to avoid a "convoy" of thieves hammering the queues of a
    // sequence of threads in a deterministic way.
    std::ranges::shuffle(victim_order, std::minstd_rand(thread_id));

    while (queue.pop(u)) {
    restart_with_stolen_work:
        auto [visited_mask, node_index, total_weight, ignore_slopes] = u;
        if (node_index == graph.goal_index) {
            auto &sol = ignore_slopes ? solutions[1] : solutions[0];
            atomic_store_max(sol, static_cast<size_t>(total_weight));
            continue;
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
                queue.push(State{
                    .visited_mask = visited_mask | (UINT64_C(1) << node_index),
                    .node_index = next_index,
                    .total_weight = static_cast<uint16_t>(total_weight + node.weights[i]),
                    .ignore_slopes = ignore_slopes,
                });
        }
    }

    // Out of items. If there are only thieves left, we are done; otherwise,
    // try to steal something from a worker thread.
    n_thieves.fetch_add(1, std::memory_order_relaxed);
    while (n_thieves.load(std::memory_order_relaxed) != n_threads) {
        for (const size_t i : victim_order) {
            if (all_queues[i].steal(u)) {
                n_thieves.fetch_sub(1, std::memory_order_relaxed);
                goto restart_with_stolen_work;
            }
        }
    }
}

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

    {
        ThreadPool &pool = ThreadPool::get();
        std::vector<WorkQueue> queues(pool.num_threads());

        // Push root tasks for part 1 and part 2.
        auto &q = queues[0];
        q.push(State{.node_index = graph.start_index, .ignore_slopes = false});
        q.push(State{.node_index = graph.start_index, .ignore_slopes = true});

        std::atomic<size_t> n_thieves = 0;
        std::array<std::atomic<size_t>, 2> solutions{};
        pool.for_each_thread([&](size_t thread_id) noexcept {
            search(thread_id, pool.num_threads(), graph, n_thieves, solutions.data(),
                   queues.data());
        });

        fmt::print("{}\n{}\n", solutions[0].load() - 1, solutions[1].load() - 1);
    }
}

}
