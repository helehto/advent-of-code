#include "common.h"

namespace aoc_2024_16 {

struct alignas(4) State {
    int16_t index;
    int16_t dir;

    bool operator<(const State &other) const
    {
        return std::bit_cast<uint32_t>(*this) < std::bit_cast<uint32_t>(other);
    }
};

void run(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);
    auto grid = Matrix<char>::from_lines(lines);

    int16_t start = -1, end = -1;
    for (size_t i = 0; i < grid.size(); i++) {
        if (grid.data[i] == 'S')
            start = i;
        if (grid.data[i] == 'E')
            end = i;
    }
    ASSERT(start > 0);
    ASSERT(end > 0);

    // Bucket queue for Dijkstra (but handled somewhat peculiarly for
    // performance, see below). The index used here is the cost modulo the
    // number of buckets, chosen to be 1024 due to being a power of two for
    // cheap masking, and that we need at least 1000 since that is the largest
    // edge weight in the graph.
    std::array<std::vector<State>, 1024> buckets;
    constexpr size_t bucket_mask = buckets.size() - 1;

    // Distance matrix for Dijkstra; one distance for each state, i.e. (point,
    // direction).
    std::vector<std::array<int32_t, 4>> dist(grid.size());
    for (auto &d : dist)
        d.fill(INT32_MAX);
    auto dist_of = [&](const State &u) -> int32_t & { return dist[u.index][u.dir]; };

    // The fact that turning costs so much more than taking a step means that
    // the bucket queue will effectively be partitioned into two (nearly)
    // contiguous groups: one for all states that involve taking a single step,
    // and another for all states that involve turning. These groups will be
    // slightly less than 1000 indices apart (depending on the length of the
    // longest straight corridor in the shortest path found).
    //
    // Ordinarily, with a bucket queue we would be forced to scan ~1000 empty
    // buckets, but due to this property we can keep track of the current min
    // and max of the two groups during pathfinding, and directly skip to the
    // next group once we have exhausted the current one.
    //
    // (d, d_max) represents the min and max (exclusive) priorities of the
    // current group, while (d_next, d_max_next) is the same for the next group
    // (that is, states which are reachable from a state in the current group
    // by turning left or right and adding a cost of 1000).
    int d = 0;
    int d_max = 1;
    int d_next = 1000;
    int d_max_next = 1000;

    // Utility function for popping and returning a state with minimum cost
    // from the bucket queue.
    auto pop_minimum = [&] {
        while (true) {
            // Try to find an non-empty bucket in the current group (d, d_max).
            for (; d < d_max; d++) {
                if (auto &bucket = buckets[d & bucket_mask]; !bucket.empty()) {
                    State result = bucket.back();
                    bucket.pop_back();
                    return result;
                }
            }

            // The current group is empty. All neighboring states that involved
            // taking a single step (adding a cost of 1), but not turning, have
            // been explored. Jump to the next group, which contains the states
            // that involved turning (adding a cost of 1000).
            ASSERT_MSG(d_next != d_max_next, "No path found!?");
            d = d_next;
            d_max = d_max_next;
            d_next = d + 1000;
            d_max_next = d_next;
        }
    };

    enum { N, E, S, W };
    const int16_t direction_offset[] = {
        static_cast<int16_t>(-static_cast<int16_t>(grid.rows)),
        1,
        static_cast<int16_t>(grid.rows),
        -1,
    };

    // Set up initial state for Dijkstra.
    const State init_state{start, E};
    buckets[d].push_back(init_state);
    dist_of(init_state) = d;
    std::optional<int> lowest;

    while (true) {
        const State u = pop_minimum();

        if (lowest.has_value() && dist_of(u) > *lowest)
            break;

        // This state has already been reached by other means, ignore it. This
        // could be avoided by updating the bucket queue with a decrease-key
        // operation instead of adding redundant states, but I'm not sure that
        // will be any faster.
        if (d != dist_of(u))
            continue;

        if (u.index == end)
            lowest = d;

        // Utility function to queue exploration of a new state by adding it to
        // the queue and updating the upper bound for the appropriate group.
        auto explore = [&](const State new_state, int32_t alt, int &group_max) {
            int32_t &old = dist_of(new_state);
            if (alt <= old) {
                old = alt;
                group_max = std::max(group_max, alt + 1);
                buckets[alt & bucket_mask].push_back(new_state);
            }
        };

        // Explore straight head:
        if (int16_t q = u.index + direction_offset[u.dir]; grid.data[q] != '#')
            explore(State{q, u.dir}, d + 1, d_max);

        // Explore by turning left:
        const State turnl{u.index, static_cast<int8_t>((u.dir - 1) & 3)};
        explore(turnl, d + 1000, d_max_next);

        // Explore by turning right:
        const State turnr{u.index, static_cast<int8_t>((u.dir + 1) & 3)};
        explore(turnr, d + 1000, d_max_next);
    }

    // Figure out all minimum paths by walking backwards from the end and
    // observing which surrounding states have a minimum cost of 1 less when
    // moving backwards or 1000 less when turning. (Completely unvisited states
    // are initialized to INT32_MAX above, so are automatically excluded.)
    std::vector<State> queue{{end, N}, {end, E}, {end, S}, {end, W}};
    queue.reserve(4 * grid.size());
    std::vector<bool> visited(grid.size());
    while (!queue.empty()) {
        auto u = queue.back();
        queue.pop_back();
        visited[u.index] = true;

        const auto cost = dist_of(u);

        const State forward{
            static_cast<int16_t>(u.index - direction_offset[u.dir]),
            u.dir,
        };
        if (cost == dist_of(forward) + 1)
            queue.push_back(forward);

        const State turnl{u.index, static_cast<int16_t>((u.dir - 1) & 3)};
        if (cost == dist_of(turnl) + 1000)
            queue.push_back(turnl);

        const State turnr{u.index, static_cast<int16_t>((u.dir + 1) & 3)};
        if (cost == dist_of(turnr) + 1000)
            queue.push_back(turnr);
    }

    fmt::print("{}\n", lowest.value());
    fmt::print("{}\n", std::ranges::count(visited, true));
}

}
