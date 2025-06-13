#include "common.h"
#include "dense_map.h"

namespace aoc_2021_23 {

constexpr int HALLWAY_STOPS[] = {0, 1, 3, 5, 7, 9, 10};
constexpr int ROOMS[] = {2, 4, 6, 8};
constexpr std::pair<int, int> ROOM_ADJ[] = {{1, 2}, {2, 3}, {3, 4}, {4, 5}};

struct State {
    std::array<char, 11> hallway;
    std::array<inplace_vector<char, 4>, 4> rooms;

    constexpr State pop_room(const int room, const int dest) const
    {
        DEBUG_ASSERT(hallway[dest] == '.');

        State result = *this;
        result.hallway[dest] = rooms[room].back();
        result.rooms[room].pop_back();
        return result;
    }

    constexpr State push_room(const int src, const int room) const
    {
        DEBUG_ASSERT(hallway[src] != '.');

        State result = *this;
        result.rooms[room].push_back(result.hallway[src]);
        result.hallway[src] = '.';
        return result;
    }

    constexpr bool operator==(const State &other) const noexcept
    {
        return hallway == other.hallway && rooms == other.rooms;
    }
};

}

template <>
struct std::hash<aoc_2021_23::State> {
    size_t operator()(const aoc_2021_23::State &state) const noexcept
    {
        size_t result = 0x8f7b9c83;
        CrcHasher c;
        result ^= c(state.hallway);
        for (const auto &room : state.rooms)
            result ^= c(std::span(room));
        return result;
    }
};

namespace aoc_2021_23 {

constexpr size_t target_room(char amphipod)
{
    DEBUG_ASSERT(amphipod >= 'A' && amphipod <= 'D');
    return amphipod - 'A';
}

constexpr int weight(char amphipod)
{
    DEBUG_ASSERT(amphipod >= 'A' && amphipod <= 'D');
    return pow10i[amphipod - 'A'];
}

static small_vector<std::pair<State, int>, 4> hallway2room(const State state,
                                                           int room_capacity)
{
    small_vector<std::pair<State, int>, 4> new_states;

    for (size_t stop_index = 0; stop_index < std::size(HALLWAY_STOPS); ++stop_index) {
        const auto stop = HALLWAY_STOPS[stop_index];
        const char amphipod = state.hallway[stop];
        if (amphipod == '.')
            continue;

        const size_t room_index = target_room(amphipod);
        const int room_x = ROOMS[room_index];
        const std::span<const char> room = state.rooms[room_index];

        // Does the room contain misplaced amphipods? If so, we can't move this
        // amphipod there yet.
        if (std::ranges::any_of(room, λa(room_index != target_room(a))))
            continue;

        // Check for collisions in the hallway.
        const bool collision = [&] {
            if (room_x < stop) {
                for (int t = stop_index - 1;; --t) {
                    if (HALLWAY_STOPS[t] < room_x)
                        return false;
                    if (state.hallway[HALLWAY_STOPS[t]] != '.')
                        return true;
                }
            } else {
                for (int t = stop_index + 1;; ++t) {
                    if (HALLWAY_STOPS[t] > room_x)
                        return false;
                    if (state.hallway[HALLWAY_STOPS[t]] != '.')
                        return true;
                }
            }
        }();

        if (!collision) {
            const int slot = room_capacity - room.size();
            const int score = (std::abs(room_x - stop) + slot) * weight(amphipod);
            new_states.emplace_back(state.push_room(stop, room_index), score);
        }
    }

    return new_states;
}

static small_vector<std::pair<State, int>, 4> room2hallway(const State state,
                                                           int room_capacity)
{
    small_vector<std::pair<State, int>, 4> new_states;

    for (size_t room_index = 0; room_index < state.rooms.size(); ++room_index) {
        const int room_x = ROOMS[room_index];
        std::span<const char> room = state.rooms[room_index];

        // If the room is empty, there is nothing to move.
        if (room.empty())
            continue;

        // Does this room contain any misplaced amphipods? If not, no need to
        // move anything.
        if (std::ranges::all_of(room, λa(room_index == target_room(a))))
            continue;

        auto add = [&](size_t stop_index) {
            const int stop = HALLWAY_STOPS[stop_index];
            const int slot = room_capacity - room.size() + 1;
            const int score = (std::abs(room_x - stop) + slot) * weight(room.back());
            new_states.emplace_back(state.pop_room(room_index, stop), score);
        };

        // Generate moves for the top amphipod.
        {
            size_t stop_index = ROOM_ADJ[room_index].first;
            while (stop_index < std::size(HALLWAY_STOPS) &&
                   state.hallway[HALLWAY_STOPS[stop_index]] == '.')
                add(stop_index--);
        }
        {
            size_t stop_index = ROOM_ADJ[room_index].second;
            while (stop_index < std::size(HALLWAY_STOPS) &&
                   state.hallway[HALLWAY_STOPS[stop_index]] == '.')
                add(stop_index++);
        }
    }

    return new_states;
}

static small_vector<std::pair<State, int>, 4> genmoves(const State state,
                                                       int room_capacity)
{
    small_vector<std::pair<State, int>, 4> result;
    result.append_range(room2hallway(state, room_capacity));
    result.append_range(hallway2room(state, room_capacity));
    return result;
}

struct PqState {
    int score;
    State u;
};

static int solve(const State state, size_t room_capacity)
{
    small_vector<PqState, 64> q;
    q.emplace_back(0, state);

    dense_map<State, int> d;
    d.emplace(state, 0);

    State goal;

    while (!q.empty()) {
        std::ranges::pop_heap(q, λab(a.score > b.score));
        const PqState p = q.back();
        q.pop_back();

        auto u = p.u;

        if (p.score != d.at(u))
            continue;

        if (std::ranges::all_of(u.hallway, λa(a == '.')) &&
            std::ranges::all_of(u.rooms[0], λa(a == 'A')) &&
            std::ranges::all_of(u.rooms[1], λa(a == 'B')) &&
            std::ranges::all_of(u.rooms[2], λa(a == 'C')) &&
            std::ranges::all_of(u.rooms[3], λa(a == 'D')))
            return d.at(u);

        for (const auto &[v, weight] : genmoves(u, room_capacity)) {
            const int alt = p.score + weight;
            auto [it, inserted] = d.emplace(v, 1'000'000'000);
            if (alt < it->second) {
                it->second = alt;
                q.emplace_back(alt, v);
                std::ranges::push_heap(q, λab(a.score > b.score));
            }
        }
    }

    ASSERT_MSG(false, "No solution found!?");
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    State state;
    state.hallway.fill('.');

    state.rooms[0].push_back(lines[3][3]);
    state.rooms[0].push_back(lines[2][3]);
    state.rooms[1].push_back(lines[3][5]);
    state.rooms[1].push_back(lines[2][5]);
    state.rooms[2].push_back(lines[3][7]);
    state.rooms[2].push_back(lines[2][7]);
    state.rooms[3].push_back(lines[3][9]);
    state.rooms[3].push_back(lines[2][9]);
    fmt::print("{}\n", solve(state, 2));

    state.rooms[0] = {state.rooms[0][0], 'D', 'D', state.rooms[0][1]};
    state.rooms[1] = {state.rooms[1][0], 'B', 'C', state.rooms[1][1]};
    state.rooms[2] = {state.rooms[2][0], 'A', 'B', state.rooms[2][1]};
    state.rooms[3] = {state.rooms[3][0], 'C', 'A', state.rooms[3][1]};
    fmt::print("{}\n", solve(state, 4));
}

}
