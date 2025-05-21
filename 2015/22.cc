#include "common.h"

namespace aoc_2015_22 {

struct State {
    int16_t boss_hp;
    int16_t boss_damage;
    int16_t player_hp = 50;
    int16_t mana = 500;
    int16_t spent = 0;
    int16_t shield_until = -1;
    int16_t poison_until = -1;
    int16_t recharge_until = -1;
    bool hard_mode = false;
};

constexpr std::optional<State> cast_spell(State state, int cost)
{
    if (state.mana < cost)
        return std::nullopt;

    state.mana -= cost;
    state.spent += cost;
    return state;
}

constexpr int player_turn(int turn, int &minimum_spent, State state);

constexpr void apply_common_effects(int turn, State &state)
{
    if (turn <= state.poison_until)
        state.boss_hp -= 3;
    if (turn <= state.recharge_until)
        state.mana += 101;
}

constexpr int boss_turn(int turn, int &minimum_spent, State state)
{
    if (state.spent > minimum_spent)
        return state.spent;

    apply_common_effects(turn, state);

    if (state.boss_hp <= 0)
        return minimum_spent = std::min<int>(minimum_spent, state.spent);

    const int armor = (turn <= state.shield_until) ? 7 : 0;
    const int damage = std::max(1, state.boss_damage - armor);

    state.player_hp -= damage;
    return player_turn(turn + 1, minimum_spent, state);
}

constexpr int player_turn(int turn, int &minimum_spent, State state)
{
    if (state.spent > minimum_spent)
        return state.spent;

    state.player_hp -= state.hard_mode;
    apply_common_effects(turn, state);

    if (state.player_hp <= 0)
        return INT_MAX;
    if (state.boss_hp <= 0)
        return minimum_spent = std::min<int>(minimum_spent, state.spent);

    int result = INT_MAX;

    // Poison:
    if (turn >= state.poison_until) {
        if (auto next_state = cast_spell(state, 173)) {
            next_state->poison_until = turn + 6;
            result = std::min(result, boss_turn(turn + 1, minimum_spent, *next_state));
        }
    }

    // Recharge:
    if (turn >= state.recharge_until) {
        if (auto next_state = cast_spell(state, 229)) {
            next_state->recharge_until = turn + 5;
            result = std::min(result, boss_turn(turn + 1, minimum_spent, *next_state));
        }
    }

    // Shield:
    if (turn >= state.shield_until) {
        if (auto next_state = cast_spell(state, 113)) {
            next_state->shield_until = turn + 6;
            result = std::min(result, boss_turn(turn + 1, minimum_spent, *next_state));
        }
    }

    // Magic Missile:
    if (auto next_state = cast_spell(state, 53)) {
        next_state->boss_hp -= 4;
        result = std::min(result, boss_turn(turn + 1, minimum_spent, *next_state));
    }

    // Drain:
    if (auto next_state = cast_spell(state, 73)) {
        next_state->boss_hp -= 2;
        next_state->player_hp += 2;
        result = std::min(result, boss_turn(turn + 1, minimum_spent, *next_state));
    }

    return result;
}

void run(std::string_view buf)
{
    const auto [boss_hp, boss_damage] = find_numbers_n<int, 2>(buf);

    auto solve = [&](bool hard_mode) {
        State initial_state{
            .boss_hp = static_cast<int16_t>(boss_hp),
            .boss_damage = static_cast<int16_t>(boss_damage),
            .hard_mode = hard_mode,
        };
        int minimum_spent = INT_MAX;
        return player_turn(0, minimum_spent, initial_state);
    };

    fmt::print("{}\n{}\n", solve(false), solve(true));
}

}
