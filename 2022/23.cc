#include "common.h"
#include "uint256.h"

namespace aoc_2022_23 {

static std::tuple<std::vector<uint256>, size_t, size_t> parse_input(std::string_view buf)
{
    std::vector<Vec2i> elves;
    for (size_t i = 0; std::string_view s : split_lines(buf)) {
        for (size_t j = 0; j < s.size(); j++)
            if (s[j] == '#')
                elves.push_back(Vec2i(j, i));
        i++;
    }

    auto min_x = std::ranges::min(elves, {}, λa(a.x)).x;
    auto max_x = std::ranges::max(elves, {}, λa(a.x)).x;
    auto min_y = std::ranges::min(elves, {}, λa(a.y)).y;
    auto max_y = std::ranges::max(elves, {}, λa(a.y)).y;

    const size_t dy = max_y - min_y + 1;
    const size_t dx = max_x - min_x + 1;
    ASSERT(3 * dx < 256);

    // In the worst case, we expand by a factor 3 in each direction (this would
    // be guaranteed to leave a space between each elf, which would cause them
    // to stop moving).
    std::vector<uint256> rows((dy + 1) * 3);
    const size_t y_offset = (rows.size() - dy) / 2;
    const size_t x_offset = (256 - dx) / 2;
    for (const auto [x, y] : elves) {
        // center the input in the pre-sized grid
        auto yy = y - min_y + y_offset;
        auto xx = x - min_x + x_offset;
        ASSERT(xx < 256);
        rows[yy].set_bit(xx);
    }

    return {rows, y_offset, y_offset + dy - 1};
}

static uint256 shift_west_1(const uint256 &v)
{
    return v.shift_right1();
}

static uint256 shift_east_1(const uint256 &v)
{
    return v.shift_left1();
}

/// Propose moves for all elves in the grid, filling in the proposals_* arrays,
/// which are bitmasks of potential target squares for each direction.
static void propose_moves(std::span<const uint256> rows,
                          const int round_num,
                          uint256 *proposals_n,
                          uint256 *proposals_s,
                          uint256 *proposals_w,
                          uint256 *proposals_e,
                          const size_t min_y,
                          const size_t max_y)
{
    // Carry these through each loop iteration; shifting is relatively
    // expensive.
    uint256 nw = shift_east_1(rows[min_y - 1]);
    uint256 ne = shift_west_1(rows[min_y - 1]);
    uint256 w = shift_east_1(rows[min_y]);
    uint256 e = shift_west_1(rows[min_y]);

    for (size_t i = min_y; i <= max_y; ++i) {
        // Neighboring cells.
        const uint256 n = rows[i - 1];
        const uint256 s = rows[i + 1];
        const uint256 sw = shift_east_1(s);
        const uint256 se = shift_west_1(s);

        const uint256 has_neighbors = n | s | w | e | nw | ne | sw | se;

        const uint256 can_move_n = rows[i] & has_neighbors & ~(n | ne | nw);
        const uint256 can_move_s = rows[i] & has_neighbors & ~(s | se | sw);
        const uint256 can_move_w = rows[i] & has_neighbors & ~(w | nw | sw);
        const uint256 can_move_e = rows[i] & has_neighbors & ~(e | ne | se);

        switch (round_num % 4) {
        case 0: // N -> S -> W -> E
            proposals_n[i - 1] = can_move_n;
            proposals_s[i + 1] = can_move_s & ~can_move_n;
            proposals_w[i] = shift_west_1(can_move_w & ~can_move_n & ~can_move_s);
            proposals_e[i] =
                shift_east_1(can_move_e & ~can_move_n & ~can_move_s & ~can_move_w);
            break;

        case 1: // S -> W -> E -> N
            proposals_s[i + 1] = can_move_s;
            proposals_w[i] = shift_west_1(can_move_w & ~can_move_s);
            proposals_e[i] = shift_east_1(can_move_e & ~can_move_s & ~can_move_w);
            proposals_n[i - 1] = can_move_n & ~can_move_s & ~can_move_w & ~can_move_e;
            break;

        case 2: // W -> E -> N -> S
            proposals_w[i] = shift_west_1(can_move_w);
            proposals_e[i] = shift_east_1(can_move_e & ~can_move_w);
            proposals_n[i - 1] = can_move_n & ~can_move_w & ~can_move_e;
            proposals_s[i + 1] = can_move_s & ~can_move_w & ~can_move_e & ~can_move_n;
            break;

        case 3: // E -> N -> S -> W
            proposals_e[i] = shift_east_1(can_move_e);
            proposals_n[i - 1] = can_move_n & ~can_move_e;
            proposals_s[i + 1] = can_move_s & ~can_move_e & ~can_move_n;
            proposals_w[i] =
                shift_west_1(can_move_w & ~can_move_e & ~can_move_n & ~can_move_s);
            break;
        }

        nw = std::exchange(w, sw);
        ne = std::exchange(e, se);
    }
}

/// Execute the proposed moves, returning true if any elf moved.
static bool execute_moves(std::span<uint256> rows,
                          const uint256 *proposals_n,
                          const uint256 *proposals_s,
                          const uint256 *proposals_w,
                          const uint256 *proposals_e,
                          size_t &min_y,
                          size_t &max_y)
{
    uint256 moved = 0;
    size_t new_min_y = min_y;
    size_t new_max_y = max_y;

    // Handle the row north of the current top row separately, to avoid bounds
    // checking in the main loop below; this is greatly simplified by the fact
    // that something cannot move into this row from above, and no sideways
    // moves are possible either.
    {
        const uint256 proposals = proposals_n[min_y - 1];
        moved |= proposals;
        rows[min_y - 1] |= proposals;
        rows[min_y] &= ~proposals;
        new_min_y = proposals ? min_y - 1 : min_y;
    }

    // Handle the row below the current bottom row separately as well.
    {
        const uint256 proposals = proposals_s[max_y + 1];
        moved |= proposals;
        rows[max_y + 1] |= proposals;
        rows[max_y] &= ~proposals;
        new_max_y = proposals ? max_y + 1 : max_y;
    }

    // Main loop: handle all rows between min_y and max_y.
    for (size_t i = min_y; i <= max_y; ++i) {
        const uint256 to_move_n = proposals_n[i] & ~proposals_s[i];
        const uint256 to_move_s = proposals_s[i] & ~proposals_n[i];
        const uint256 to_move_w = proposals_w[i] & ~proposals_e[i];
        const uint256 to_move_e = proposals_e[i] & ~proposals_w[i];
        const uint256 all_moves = to_move_n | to_move_s | to_move_w | to_move_e;
        moved |= all_moves;
        rows[i] |= all_moves;
        rows[i + 1] &= ~to_move_n;
        rows[i - 1] &= ~to_move_s;
        rows[i] &= ~shift_east_1(to_move_w) & ~shift_west_1(to_move_e);
    }

    min_y = new_min_y;
    max_y = new_max_y;

    return moved;
}

static bool step(std::span<uint256> rows,
                 int round_num,
                 uint256 *proposals_n,
                 uint256 *proposals_s,
                 uint256 *proposals_w,
                 uint256 *proposals_e,
                 size_t &min_y,
                 size_t &max_y)
{
    propose_moves(rows, round_num, proposals_n, proposals_s, proposals_w, proposals_e,
                  min_y, max_y);
    return execute_moves(rows, proposals_n, proposals_s, proposals_w, proposals_e, min_y,
                         max_y);
}

static size_t count_tiles(std::span<const uint256> rows)
{
    size_t min_i = rows.size();
    size_t max_i = 0;
    size_t min_j = 256;
    size_t max_j = 0;
    size_t elves = 0;

    for (size_t i = 0; i < rows.size(); ++i) {
        if (rows[i]) {
            min_i = std::min(min_i, i);
            max_i = std::max(max_i, i);
            min_j = std::min<size_t>(min_j, countr_zero(rows[i]));
            max_j = std::max<size_t>(max_j, 256 - countl_zero(rows[i]));
            elves += popcount(rows[i]);
        }
    }

    return (max_i - min_i + 1) * (max_j - min_j) - elves;
}

void run(std::string_view buf)
{
    auto [rows, min_y, max_y] = parse_input(buf);

    std::vector<uint256> proposals_n(rows.size());
    std::vector<uint256> proposals_s(rows.size());
    std::vector<uint256> proposals_w(rows.size());
    std::vector<uint256> proposals_e(rows.size());

    // dump(rows);
    int round = 0;
    for (; round < 10; ++round)
        step(rows, round, proposals_n.data(), proposals_s.data(), proposals_w.data(),
             proposals_e.data(), min_y, max_y);
    fmt::print("{}\n", count_tiles(rows));

    for (;; ++round) {
        if (!step(rows, round, proposals_n.data(), proposals_s.data(), proposals_w.data(),
                  proposals_e.data(), min_y, max_y))
            break;
    }
    fmt::print("{}\n", round + 1);
}

}
