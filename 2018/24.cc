#include "common.h"
#include "small_vector.h"

namespace aoc_2018_24 {

enum {
    IMMUNE_SYSTEM,
    INFECTION,
};

struct Group {
    int units;
    int hp_per_unit;
    int damage_per_unit;
    int damage_type;
    int initiative;
    int immune_mask;
    int weakness_mask;
    int faction;

    constexpr int effective_power() const { return units * damage_per_unit; }
};

static Group parse_group(std::string_view line, int faction)
{
    auto parse_damage_type = [](std::string_view type) -> int {
        if (type == "radiation")
            return 1 << 0;
        else if (type == "slashing")
            return 1 << 1;
        else if (type == "bludgeoning")
            return 1 << 2;
        else if (type == "cold")
            return 1 << 3;
        else if (type == "fire")
            return 1 << 4;
        else
            ASSERT_MSG(false, "Unknown damage type '{}'!?", type);
    };

    std::vector<std::string_view> tokens;
    auto parse_damage_type_list = [&](std::string_view list) -> int {
        int result = 0;

        for (std::string_view type : split(list, tokens, ','))
            result |= parse_damage_type(strip(type));

        return result;
    };

    const auto [units, hp_per_unit, damage, initiative] = find_numbers_n<int, 4>(line);
    Group group{
        .units = units,
        .hp_per_unit = hp_per_unit,
        .damage_per_unit = damage,
        .initiative = initiative,
        .faction = faction,
    };

    {
        size_t i = line.find(" damage at");
        ASSERT(i != std::string_view::npos);
        size_t j = line.rfind(' ', i - 1);
        ASSERT(j != std::string_view::npos);
        group.damage_type = parse_damage_type(line.substr(j + 1, i - j - 1));
    }

    if (size_t i = line.find("immune to "); i != std::string_view::npos) {
        i += 10;
        size_t j = line.find_first_of(";)", i);
        ASSERT(j != std::string_view::npos);
        group.immune_mask = parse_damage_type_list(line.substr(i, j - i));
    }

    if (size_t i = line.find("weak to "); i != std::string_view::npos) {
        i += 7;
        size_t j = line.find_first_of(";)", i);
        ASSERT(j != std::string_view::npos);
        group.weakness_mask = parse_damage_type_list(line.substr(i, j - i));
    }

    ASSERT((group.immune_mask & group.weakness_mask) == 0);
    return group;
}

static int compute_damage(const Group &attacker, const Group &defender)
{
    if (attacker.damage_type & defender.immune_mask)
        return 0;

    int damage = attacker.effective_power();
    if (attacker.damage_type & defender.weakness_mask)
        damage *= 2;

    return damage;
}

static small_vector<std::pair<Group *, Group *>, 32>
target_selection(std::span<Group> groups)
{
    small_vector<Group *, 32> viable_targets;
    for (Group &group : groups)
        viable_targets.push_back(&group);

    auto better_target = [&](const Group &attacker, const Group &a, const Group &b) {
        const auto a_damage = compute_damage(attacker, a);
        const auto b_damage = compute_damage(attacker, b);
        return std::tuple(a_damage, a.effective_power(), a.initiative) >
               std::tuple(b_damage, b.effective_power(), b.initiative);
    };

    auto select_target = [&](const Group &attacker) {
        Group *result = nullptr;

        for (Group *candidate : viable_targets) {
            if (attacker.faction != candidate->faction &&
                (!result || better_target(attacker, *candidate, *result)))
                result = candidate;
        }

        return result;
    };

    small_vector<Group *, 32> selection_order;
    for (size_t i = 0; i < groups.size(); ++i)
        selection_order.push_back(&groups[i]);
    auto proj = λa(std::tuple(a->effective_power(), a->initiative));
    std::ranges::sort(selection_order, λab(a > b), proj);

    small_vector<std::pair<Group *, Group *>, 32> selected_targets;
    for (Group *attacker : selection_order) {
        if (Group *defender = select_target(*attacker)) {
            if (compute_damage(*attacker, *defender) > 0) {
                selected_targets.emplace_back(attacker, defender);
                erase(viable_targets, defender);
            }
        }
    }

    return selected_targets;
}

static size_t attacking_phase(small_vector_base<Group> &groups,
                              std::span<std::pair<Group *, Group *>> order)
{
    std::ranges::sort(order, λab(a.first->initiative > b.first->initiative));

    for (auto &[attacker, defender] : order) {
        if (attacker->units <= 0 || defender->units <= 0)
            continue;

        const int damage = compute_damage(*attacker, *defender);
        defender->units -= std::min(defender->units, damage / defender->hp_per_unit);
    }

    auto dead = std::ranges::partition(groups, λa(a.units > 0));
    groups.erase(dead.begin(), groups.end());
    return std::ranges::size(dead);
}

struct FightResult {
    int winner;
    int units_left;
};

static FightResult fight(small_vector<Group, 32> groups, int boost = 0)
{
    for (Group &group : groups)
        if (group.faction == IMMUNE_SYSTEM)
            group.damage_per_unit += boost;

    small_vector<Group, 32> prev;

    while (true) {
        int factions_present = 0;
        for (Group &group : groups)
            factions_present |= 1 << group.faction;
        if (factions_present != 0b11) {
            return FightResult((factions_present & 1) ? IMMUNE_SYSTEM : INFECTION,
                               std::ranges::fold_left(groups, 0, λab(a + b.units)));
        }

        auto order = target_selection(groups);
        if (attacking_phase(groups, order) == 0) {
            auto proj = λa(std::pair(a.faction, a.units));
            std::ranges::sort(prev, {}, proj);
            std::ranges::sort(groups, {}, proj);
            if (std::ranges::equal(prev, groups, {}, proj, proj)) {
                // Actually a draw, but for the purposes of binary search, it's
                // equivalent to the infection winning.
                return FightResult(INFECTION, 0);
            }
        }

        prev = groups;
    }
}

static int part2(small_vector<Group, 32> groups)
{
    int hi = 1;

    while (fight(groups, hi).winner != IMMUNE_SYSTEM)
        hi *= 2;

    int lo = hi / 2;
    while (lo + 1 < hi) {
        const int mid = (lo + hi) / 2;
        if (fight(groups, mid).winner == IMMUNE_SYSTEM)
            hi = mid;
        else
            lo = mid;
    }

    return fight(groups, hi).units_left;
}

void run(std::string_view buf)
{
    const auto lines = split_lines(buf);
    auto separator = std::ranges::find(lines, "");

    small_vector<Group, 32> groups;
    for (auto it = lines.begin() + 1; it != separator; ++it)
        groups.push_back(parse_group(*it, IMMUNE_SYSTEM));
    for (auto it = separator + 2; it != lines.end(); ++it)
        groups.push_back(parse_group(*it, INFECTION));

    fmt::print("{}\n", fight(groups).units_left);
    fmt::print("{}\n", part2(groups));
}

}
