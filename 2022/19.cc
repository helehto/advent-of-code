#include "common.h"
#include "dense_map.h"

namespace aoc_2022_19 {

enum {
    ORE,
    CLAY,
    OBSIDIAN,
    GEODE,
};

struct Blueprint {
    uint8_t costs[4][3];
};

struct alignas(8) CacheKey {
    std::array<uint8_t, 3> income;
    std::array<uint8_t, 3> resources;
    uint8_t pad0 = 0;
    uint8_t pad1 = 0;

    constexpr bool operator==(const CacheKey &o) const
    {
        static_assert(sizeof(*this) == 8 && alignof(*this) == 8);
        return *reinterpret_cast<const uint64_t *>(this) ==
               *reinterpret_cast<const uint64_t *>(&o);
    }
};

} // namespace aoc_2022_19

template <>
struct std::hash<aoc_2022_19::CacheKey> {
    size_t operator()(const aoc_2022_19::CacheKey &k) const
    {
        return _mm_crc32_u64(0, std::bit_cast<uint64_t>(k));
    }
};

namespace aoc_2022_19 {

struct SearchParameters {
    Blueprint blueprint;
    int target;
    std::array<uint8_t, 3> max_cost_per_material;
    dense_map<CacheKey, int> &cache;
    int largest = 0;
};

struct SearchState {
    int t = 0;
    int geodes = 0;
    std::array<uint8_t, 3> income = {1, 0, 0};
    std::array<uint8_t, 3> resources = {0, 0, 0};
};

static int search(SearchParameters &p, const SearchState &state = {})
{
    const auto &[t, geodes, income, resources] = state;

    // If we can't beat the current maximum even if we were to build a geode
    // robot every single turn from now on, ignoring resource constraints, then
    // prune this branch.
    const auto time_left = p.target - state.t;
    const auto upper_bound = state.geodes + (time_left - 1) * time_left / 2;
    if (upper_bound < p.largest)
        return 0;

    CacheKey key{income, resources};
    if (auto it = p.cache.find(key); it != end(p.cache))
        return it->second;

    int score = geodes;

    // Recursive call: wait until we can afford to build a robot of type `i`,
    // then build it.
    for (int i = 3; i >= 0; i--) {
        // If we can't ever build it with the income that we have right now,
        // skip it.
        if ((income[0] == 0 && p.blueprint.costs[i][0] != 0) ||
            (income[1] == 0 && p.blueprint.costs[i][1] != 0) ||
            (income[2] == 0 && p.blueprint.costs[i][2] != 0))
            continue;

        // Calculate the time needed to afford the robot.
        int dt = 0;
        for (int j = 0; j < 3; j++) {
            if (auto cost = p.blueprint.costs[i][j]; cost > resources[j])
                dt = std::max(dt, (cost - resources[j] + income[j] - 1) / income[j]);
        }

        // Building the robot takes a cycle.
        dt += 1;

        // Do we even have enough time to build a robot of this type? If not,
        // skip it.
        const auto t_next = t + dt;
        if (t_next >= p.target)
            continue;

        // If we are already generating more material of this type than any
        // single robot costs to build, then it isn't useful to build another
        // one - we would be generating more than we can possibly use. The
        // exception is geodes, which we always want more of.
        if (i != GEODE && income[i] >= p.max_cost_per_material[i])
            continue;

        // Fast-forward time until the point where we can afford the robot, and
        // continue recursively searching from there.
        auto new_resources = resources;
        for (int j = 0; j < 3; j++)
            new_resources[j] += income[j] * dt - p.blueprint.costs[i][j];

        auto new_income = income;
        int new_geodes;
        if (i != GEODE) {
            new_income[i] += 1;
            new_geodes = geodes;
        } else {
            new_geodes = geodes + p.target - t_next;
        }

        SearchState new_state{
            .t = t_next,
            .geodes = new_geodes,
            .income = new_income,
            .resources = new_resources,
        };
        int new_score = search(p, new_state);
        score = std::max(score, new_score);
    }

    p.cache[key] = {score};
    if (score > p.largest)
        p.largest = score;
    return score;
}

void run(std::string_view buf)
{
    std::vector<Blueprint> blueprints;

    for (std::string_view s : split_lines(buf)) {
        auto [_, oror, clor, obor, obcl, geor, geob] = find_numbers_n<int, 7>(s);
        Blueprint &b = blueprints.emplace_back();
        b.costs[ORE][ORE] = oror;
        b.costs[CLAY][ORE] = clor;
        b.costs[OBSIDIAN][ORE] = obor;
        b.costs[OBSIDIAN][CLAY] = obcl;
        b.costs[GEODE][ORE] = geor;
        b.costs[GEODE][OBSIDIAN] = geob;
    }

    int part1 = 0;
    int part2 = 1;
    dense_map<CacheKey, int> cache;
    for (size_t i = 0; i < blueprints.size(); i++) {
        std::array<uint8_t, 3> max_cost_per_material;
        for (size_t j = 0; j < 4; j++) {
            for (size_t k = 0; k < 3; k++) {
                max_cost_per_material[k] =
                    std::max(max_cost_per_material[k], blueprints[i].costs[j][k]);
            }
        }

        SearchParameters p = {
            .blueprint = blueprints[i],
            .target = 24,
            .max_cost_per_material = max_cost_per_material,
            .cache = cache,
        };

        p.cache.clear();
        p.largest = 0;
        part1 += (i + 1) * search(p);

        if (i < 3) {
            p.cache.clear();
            p.target = 32;
            p.largest = 0;
            part2 *= search(p);
        }
    }

    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}

}
