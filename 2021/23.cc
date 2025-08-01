#include "common.h"
#include "dense_map.h"
#include "monotonic_bucket_queue.h"

namespace aoc_2021_23 {

struct State {
    uint64_t hallway = 0;
    std::array<uint16_t, 4> rooms;

    /// Move the top amphipod from `room` to slot `dest` in the hallway.
    State pop_room(const int room, const int dest) const
    {
        const int shift = _tzcnt_u32(rooms[room]) & ~3;
        const uint64_t pod = (rooms[room] >> shift) & 0xf;
        State result = *this;
        result.hallway |= pod << (4 * dest);
        result.rooms[room] &= ~(0b1111 << shift);
        return result;
    }

    /// Move the amphipod at slot `src` in the hallway to the first available
    /// slot in the given room.
    State push_room(const int src, const int room, const int room_capacity) const
    {
        const uint64_t mask = UINT64_C(0b1111) << (4 * src);
        const auto n = rooms[room] ? _tzcnt_u32(rooms[room]) : room_capacity * 4;
        State result = *this;
        result.hallway &= ~mask;
        result.rooms[room] |= ((hallway >> (4 * src)) & 0b1111) << ((n - 4) & ~3);
        return result;
    }

    constexpr bool operator==(const State &other) const noexcept
    {
        return hallway == other.hallway &&
               std::bit_cast<uint64_t>(rooms) == std::bit_cast<uint64_t>(other.rooms);
    }
};

constexpr uint64_t bits_between(int hi, int lo)
{
    const uint64_t himask = (UINT64_C(1) << (hi + 1)) - 1;
    const uint64_t lomask = (UINT64_C(1) << lo) - 1;
    return himask & ~lomask;
}

static bool has_misplaced_amphipods(const State &state, const size_t dest_room)
{
    const uint32_t expected_mask = 0x8888 | (0x1111 * dest_room);
    uint32_t occupancy_mask = state.rooms[dest_room] & 0x8888;
    occupancy_mask |= occupancy_mask >> 1;
    occupancy_mask |= occupancy_mask >> 2;
    return (state.rooms[dest_room] & occupancy_mask) != (expected_mask & occupancy_mask);
}

static void hallway2room(inplace_vector<std::pair<State, int>, 64> &out,
                         const State &state,
                         int room_capacity)
{
    const uint64_t hallway_occupancy = _pext_u64(state.hallway, 0x8888888888888888);

    for (uint64_t m = hallway_occupancy; m; m &= m - 1) {
        const int stop = std::countr_zero(m);
        const size_t dest_room = _bextr_u64(state.hallway, 4 * stop, 3);

        // Does the room contain misplaced amphipods? If so, we can't move this
        // amphipod there yet.
        if (has_misplaced_amphipods(state, dest_room))
            continue;

        // Check for collisions in the hallway.
        const int room_x = 2 * (dest_room + 1);
        uint64_t collision_mask = room_x < stop ? bits_between(stop - 1, room_x)
                                                : bits_between(room_x, stop + 1);
        uint64_t expanded_collision_mask = _pdep_u64(collision_mask, 0x8888888888888888);

        if ((state.hallway & expanded_collision_mask) == 0) {
            const int slot = state.rooms[dest_room] == 0
                                 ? room_capacity
                                 : std::countr_zero(state.rooms[dest_room]) / 4;
            const int score = (std::abs(room_x - stop) + slot) * pow10i[dest_room];
            out.unchecked_emplace_back(state.push_room(stop, dest_room, room_capacity),
                                       score);
        }
    }
}

static void room2hallway(inplace_vector<std::pair<State, int>, 64> &out,
                         const State &state)
{
    const uint32_t hallway_occupancy = _pext_u64(state.hallway, 0x8888888888888888);

    for (size_t src_room = 0; src_room < state.rooms.size(); ++src_room) {
        // Does this room contain any misplaced amphipods? If not, no need to
        // move anything. (This also covers the case of an empty room.)
        if (!has_misplaced_amphipods(state, src_room))
            continue;

        const int slot = std::countr_zero(state.rooms[src_room]) / 4;
        const int room_x = 2 * (src_room + 1);
        const int weight = pow10i[_bextr_u32(state.rooms[src_room], 4 * slot, 3)];

        // Generate moves to the left:
        const uint32_t lstops = _bzhi_u32(0b11010101011, room_x);
        const uint32_t lblocked = 32 - std::countl_zero(hallway_occupancy & lstops);
        const uint32_t lmoves = lstops & (UINT32_MAX << lblocked);

        // Generate moves to the right:
        const uint32_t rmask = UINT32_MAX << (room_x + 1);
        const uint32_t rstops = 0b11010101011 & rmask;
        const uint32_t rblocked = std::countr_zero(hallway_occupancy & rstops);
        const uint32_t rmoves = _bzhi_u32(rstops, rblocked);

        for (uint32_t m = lmoves | rmoves; m; m &= m - 1) {
            const int stop = std::countr_zero(m);
            const int score = (std::abs(room_x - stop) + slot + 1) * weight;
            out.unchecked_emplace_back(state.pop_room(src_room, stop), score);
        }
    }
}

constexpr std::array<uint16_t, 4> goal_rooms2{
    0b1000'1000,
    0b1001'1001,
    0b1010'1010,
    0b1011'1011,
};

constexpr std::array<uint16_t, 4> goal_rooms4{
    0b1000'1000'1000'1000,
    0b1001'1001'1001'1001,
    0b1010'1010'1010'1010,
    0b1011'1011'1011'1011,
};

static int solve(const State state, size_t room_capacity)
{
    MonotonicBucketQueue<State> q(16384);
    q.emplace(0, state);

    dense_map<State, uint32_t, CrcHasher> d;
    d.reserve(100'000);
    d.emplace(state, 0);

    const State goal{
        .hallway = 0,
        .rooms = room_capacity == 2 ? goal_rooms2 : goal_rooms4,
    };

    inplace_vector<std::pair<State, int>, 64> moves;

    while (std::optional<State> u = q.pop()) {
        if (*u == goal)
            return q.current_priority();

        moves.clear();
        room2hallway(moves, *u);
        hallway2room(moves, *u, room_capacity);

        for (const auto &[v, weight] : moves) {
            const size_t alt = q.current_priority() + weight;
            if (auto [it, _] = d.emplace(v, 1'000'000'000); alt < it->second) {
                it->second = alt;
                q.emplace(alt, v);
            }
        }
    }

    ASSERT_MSG(false, "No solution found!?");
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    State state;
    state.hallway = 0;

    state.rooms[0] = (0b1000 | (lines[3][3] - 'A')) << 4 | (0b1000 | (lines[2][3] - 'A'))
                                                               << 0;
    state.rooms[1] = (0b1000 | (lines[3][5] - 'A')) << 4 | (0b1000 | (lines[2][5] - 'A'))
                                                               << 0;
    state.rooms[2] = (0b1000 | (lines[3][7] - 'A')) << 4 | (0b1000 | (lines[2][7] - 'A'))
                                                               << 0;
    state.rooms[3] = (0b1000 | (lines[3][9] - 'A')) << 4 | (0b1000 | (lines[2][9] - 'A'))
                                                               << 0;

    fmt::print("{}\n", solve(state, 2));

    state.rooms[0] =
        (state.rooms[0] & 0xf) | (state.rooms[0] & 0xf0) << 8 | 0b1011'1011'0000; // DD
    state.rooms[1] =
        (state.rooms[1] & 0xf) | (state.rooms[1] & 0xf0) << 8 | 0b1001'1010'0000; // BC
    state.rooms[2] =
        (state.rooms[2] & 0xf) | (state.rooms[2] & 0xf0) << 8 | 0b1000'1001'0000; // AB
    state.rooms[3] =
        (state.rooms[3] & 0xf) | (state.rooms[3] & 0xf0) << 8 | 0b1010'1000'0000; // CA

    fmt::print("{}\n", solve(state, 4));
}

}
