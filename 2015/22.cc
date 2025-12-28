#include "common.h"
#include "thread_pool.h"
#include <mutex>
#include <random>

namespace aoc_2015_22 {

struct alignas(16) State {
    uint8_t turn = 0;
    uint8_t flags = 0;
    uint8_t boss_hp;
    uint8_t boss_damage;
    int16_t player_hp = 50;
    int16_t mana = 500;
    int16_t spent = 0;
    int8_t shield_until = -1;
    int8_t poison_until = -1;
    int8_t recharge_until = -1;
    int8_t player_hp_decrement = 0;
};
static_assert(sizeof(State) == 16);

static void apply_common_effects(State &state)
{
    if (state.turn <= state.poison_until)
        state.boss_hp -= 3;
    if (state.turn <= state.recharge_until)
        state.mana += 101;
}

static void do_boss_turn(State &state)
{
    DEBUG_ASSERT(state.turn < 100 && state.turn % 2 == 1);
    apply_common_effects(state);
    const int armor = (state.turn <= state.shield_until) ? 7 : 0;
    const int damage = std::max(1, state.boss_damage - armor);
    state.player_hp -= damage;
    state.turn++;
}

static void do_player_preturn(State &state)
{
    DEBUG_ASSERT(state.turn < 100 && state.turn % 2 == 0);
    state.player_hp -= state.player_hp_decrement;
    apply_common_effects(state);
}

static size_t do_player_turn(State *__restrict__ out, const State &state)
{
    DEBUG_ASSERT(state.turn < 100 && state.turn % 2 == 0);
    DEBUG_ASSERT(state.player_hp > 0);
    DEBUG_ASSERT(state.boss_hp > 0);

    size_t n = 0;

    auto try_cast_spell = [&](int cost) {
        if (state.mana < cost)
            return false;

        auto &next_state = out[n++];
        next_state = state;
        next_state.mana -= cost;
        next_state.spent += cost;
        return true;
    };

    // Poison:
    if (state.turn >= state.poison_until && try_cast_spell(173)) {
        auto &next_state = out[n - 1];
        next_state.poison_until = state.turn + 6;
        next_state.turn++;
    }

    // Recharge:
    if (state.turn >= state.recharge_until && try_cast_spell(229)) {
        auto &next_state = out[n - 1];
        next_state.recharge_until = state.turn + 5;
        next_state.turn++;
    }

    // Shield:
    if (state.turn >= state.shield_until && try_cast_spell(113)) {
        auto &next_state = out[n - 1];
        next_state.shield_until = state.turn + 6;
        next_state.turn++;
    }

    // Magic Missile:
    if (try_cast_spell(53)) {
        auto &next_state = out[n - 1];
        next_state.boss_hp -= 4;
        next_state.turn++;
    }

    // Drain:
    if (try_cast_spell(73)) {
        auto &next_state = out[n - 1];
        next_state.boss_hp -= 2;
        next_state.player_hp += 2;
        next_state.turn++;
    }

    return n;
}

void run(std::string_view buf)
{
    const auto [boss_hp, boss_damage] = find_numbers_n<uint8_t, 2>(buf);
    ThreadPool &pool = ThreadPool::get();
    ForkPool<State> fork_pool(pool.num_threads());
    std::atomic<int> solutions[2] = {INT_MAX, INT_MAX};

    fork_pool.push({
        State{
            .boss_hp = boss_hp,
            .boss_damage = boss_damage,
            .player_hp_decrement = 0,
        },
        State{
            .boss_hp = boss_hp,
            .boss_damage = boss_damage,
            .player_hp_decrement = 1,
        },
    });

    fork_pool.run(pool, [&](State state, small_vector_base<State> &next_states) {
        DEBUG_ASSERT(state.turn < 100);

        auto state_done = [&](const State &s) {
            if (s.boss_hp <= 0) {
                atomic_store_min<int>(solutions[s.player_hp_decrement], s.spent);
                return true;
            }
            return s.player_hp <= 0;
        };

        if (state.spent >= solutions[state.player_hp_decrement].load())
            return;

        do_player_preturn(state);
        if (state_done(state))
            return;

        std::array<State, 5> candidates;
        size_t n = do_player_turn(candidates.data(), state);

        for (size_t i = 0; i < n; i++) {
            State &candidate = candidates[i];
            if (state_done(candidate))
                continue;

            do_boss_turn(candidate);
            if (state_done(candidate))
                continue;

            next_states.push_back(candidate);
        }
    });

    fmt::print("{}\n{}\n", solutions[0].load(), solutions[1].load());
}

}
