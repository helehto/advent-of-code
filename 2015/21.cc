#include "common.h"

namespace aoc_2015_21 {

struct Stats {
    int cost;
    int damage;
    int armor;
};

constexpr Stats weapons[] = {
    {.cost = 8, .damage = 4, .armor = 0},  {.cost = 10, .damage = 5, .armor = 0},
    {.cost = 25, .damage = 6, .armor = 0}, {.cost = 40, .damage = 7, .armor = 0},
    {.cost = 74, .damage = 8, .armor = 0},
};

constexpr Stats armors[] = {
    {.cost = 0, .damage = 0, .armor = 0},  {.cost = 13, .damage = 0, .armor = 1},
    {.cost = 31, .damage = 0, .armor = 2}, {.cost = 53, .damage = 0, .armor = 3},
    {.cost = 75, .damage = 0, .armor = 4}, {.cost = 102, .damage = 0, .armor = 5},
};

constexpr Stats rings[] = {
    {.cost = 0, .damage = 0, .armor = 0},  {.cost = 25, .damage = 1, .armor = 0},
    {.cost = 50, .damage = 2, .armor = 0}, {.cost = 100, .damage = 3, .armor = 0},
    {.cost = 20, .damage = 0, .armor = 1}, {.cost = 40, .damage = 0, .armor = 2},
    {.cost = 80, .damage = 0, .armor = 3},
};

constexpr auto item_combinations_by_cost = [] {
    const size_t size =
        std::size(weapons) * std::size(armors) * std::size(rings) * std::size(rings);
    std::array<Stats, size> result;

    size_t i = 0;
    for (const Stats &w : weapons) {
        for (const Stats &a : armors) {
            for (const Stats &r1 : rings) {
                for (const Stats &r2 : rings) {
                    result[i++] = Stats{
                        .cost = w.cost + a.cost + r1.cost + r2.cost,
                        .damage = w.damage + w.armor + r1.damage + r2.damage,
                        .armor = w.armor + a.armor + r1.armor + r2.armor,
                    };
                }
            }
        }
    }

    std::ranges::sort(result, {}, λx(x.cost));
    return result;
}();

void run(std::string_view buf)
{
    auto [boss_hp, boss_damage, boss_armor] = find_numbers_n<int, 3>(buf);

    auto wins = [&](Stats our) {
        return boss_hp * std::max(boss_damage - our.armor, 1) <=
               100 * std::max(our.damage - boss_armor, 1);
    };

    for (size_t i = 0; i < item_combinations_by_cost.size(); ++i) {
        if (wins(item_combinations_by_cost[i])) {
            fmt::print("{}\n", item_combinations_by_cost[i].cost);
            break;
        }
    }

    for (size_t i = item_combinations_by_cost.size(); i--;) {
        if (!wins(item_combinations_by_cost[i])) {
            fmt::print("{}\n", item_combinations_by_cost[i].cost);
            break;
        }
    }
}

}
