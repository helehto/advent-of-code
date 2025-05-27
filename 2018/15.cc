#include "common.h"
#include "small_vector.h"

namespace aoc_2018_15 {

struct Unit {
    char ch;
    int16_t pos;
    int16_t attack_power;
    int16_t hp;
};

constexpr char enemy_of(char ch)
{
    return ch ^ ('E' ^ 'G');
}

struct Pathfinding {
    small_vector<std::pair<int16_t, int16_t>, 64> queue;
    std::vector<uint8_t> dist;
    size_t rows;
    size_t cols;

    Pathfinding(const size_t rows, const size_t cols)
        : dist(rows * cols)
        , rows(rows)
        , cols(cols)
    {
        queue.reserve(4 * (rows + cols));
    }

    constexpr std::array<int16_t, 4> neighbors_of(const int16_t pos) const
    {
        return {
            static_cast<int16_t>(pos - cols),
            static_cast<int16_t>(pos - 1),
            static_cast<int16_t>(pos + 1),
            static_cast<int16_t>(pos + cols),
        };
    }

    std::optional<int16_t> closest_target_squares(std::span<const char> grid,
                                                  const int16_t start)
    {
        queue.clear();
        queue.emplace_back(0, start);

        int min_d = INT_MAX;
        std::optional<int16_t> result;
        std::ranges::fill(dist, UINT8_MAX);

        for (size_t i = 0; i < queue.size(); ++i) {
            auto [d, u] = queue[i];
            if (d > min_d)
                break;

            if (dist[u] < UINT8_MAX)
                continue;
            dist[u] = d;

            for (int16_t v : neighbors_of(u)) {
                if (grid[v] == enemy_of(grid[start])) {
                    min_d = d;
                    result = std::min(result.value_or(INT16_MAX), u);
                } else if (d + 1 <= min_d && grid[v] == '.' && dist[v] == UINT8_MAX) {
                    queue.emplace_back(d + 1, v);
                }
            }
        }

        return result;
    }

    std::optional<int16_t> shortest_path_step(std::span<const char> grid,
                                              const int16_t start,
                                              const int16_t attacker)
    {
        queue.clear();
        queue.emplace_back(0, start);

        int min_d = INT_MAX;
        std::optional<int16_t> result;
        std::ranges::fill(dist, UINT8_MAX);

        for (size_t i = 0; i < queue.size(); ++i) {
            auto [d, u] = queue[i];
            if (d > min_d)
                break;

            if (dist[u] < UINT8_MAX)
                continue;
            dist[u] = d;

            if (std::ranges::contains(neighbors_of(attacker), u)) {
                min_d = d;
                result = std::min(result.value_or(INT16_MAX), u);
            }

            for (auto v : neighbors_of(u))
                if (d + 1 <= min_d && grid[v] == '.' && dist[v] == UINT8_MAX)
                    queue.emplace_back(d + 1, v);
        }

        return result;
    }
};

static std::optional<size_t> get_adjacent_attack_target(const Unit &attacker,
                                                        const Pathfinding &pf,
                                                        std::span<const Unit> candidates)
{
    int min_hp = INT16_MAX;
    std::optional<size_t> result;

    for (int16_t neighbor : pf.neighbors_of(attacker.pos)) {
        auto it = std::ranges::find_if(
            candidates,
            λa(a.pos == neighbor && a.ch == enemy_of(attacker.ch) && a.hp > 0));
        if (it != candidates.end() && it->hp < min_hp) {
            min_hp = it->hp;
            result = it - candidates.begin();
        }
    }

    return result;
}

static bool move_closer(std::span<char> grid, Pathfinding &pf, Unit &u)
{
    if (auto target = pf.closest_target_squares(grid, u.pos)) {
        auto steps = pf.shortest_path_step(grid, *target, u.pos);
        grid[*steps] = std::exchange(grid[u.pos], '.');
        u.pos = *steps;
        return true;
    }
    return false;
}

static bool single_round(std::span<char> grid, Pathfinding &pf, std::vector<Unit> &units)
{
    bool done = false;
    std::ranges::sort(units, {}, λa(a.pos));

    for (size_t i = 0; i < units.size(); ++i) {
        Unit &u = units[i];
        if (u.hp <= 0)
            continue;

        if (std::ranges::count_if(units, λa(a.ch == 'E' && a.hp > 0)) == 0 ||
            std::ranges::count_if(units, λa(a.ch == 'G' && a.hp > 0)) == 0) {
            done = true;
            break;
        }

        if (auto j = get_adjacent_attack_target(u, pf, units)) {
            units[*j].hp -= u.attack_power;
            if (units[*j].hp <= 0)
                grid[units[*j].pos] = '.';
        } else if (move_closer(grid, pf, u)) {
            if (auto j = get_adjacent_attack_target(u, pf, units)) {
                units[*j].hp -= u.attack_power;
                if (units[*j].hp <= 0)
                    grid[units[*j].pos] = '.';
            }
        }
    }

    units.erase(std::ranges::partition(units, λa(a.hp > 0)).begin(), units.end());
    return !done;
}

static std::pair<int, int>
fight(Matrix<char> grid, Pathfinding &pf, std::vector<Unit> units, int elf_attack_power)
{
    auto orig_elves = std::ranges::count_if(units, λa(a.ch == 'E'));

    for (Unit &u : units)
        if (u.ch == 'E')
            u.attack_power = elf_attack_power;

    int rounds = 0;
    while (true) {
        if (!single_round(grid.all(), pf, units))
            break;
        ++rounds;
    }

    int outcome = rounds * std::ranges::fold_left(units, 0, λab(a + b.hp));
    return {outcome, orig_elves - std::ranges::count_if(units, λa(a.ch == 'E'))};
}

void run(std::string_view buf)
{
    auto grid = Matrix<char>::from_lines(split_lines(buf));
    ASSERT(grid.size() < INT16_MAX);
    ASSERT(grid.rows < UINT8_MAX);
    ASSERT(grid.cols < UINT8_MAX);

    Pathfinding pf(grid.rows, grid.cols);

    std::vector<Unit> units;
    for (auto p : grid.ndindex()) {
        if (grid(p) == 'E' || grid(p) == 'G') {
            units.push_back(Unit{
                .ch = grid(p),
                .pos = static_cast<int16_t>(p.y * grid.cols + p.x),
                .attack_power = 3,
                .hp = 200,
            });
        }
    }
    fmt::print("{}\n", fight(grid, pf, units, 3).first);

    // Prepare for binary search; find an upper bound:
    int hi = 4;
    while (fight(grid, pf, units, hi).second != 0)
        hi *= 2;
    int lo = hi / 2;

    // Binary search:
    int outcome = 0;
    int casualties = 0;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        std::tie(outcome, casualties) = fight(grid, pf, units, mid);
        if (casualties)
            lo = mid + 1;
        else
            hi = mid;
    }
    fmt::print("{}\n", fight(grid, pf, units, hi).first);
}

}
