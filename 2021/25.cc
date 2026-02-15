#include "common.h"
#include "uint256.h"

namespace aoc_2021_25 {

static bool step_r(std::span<uint256> r, std::span<const uint256> d, const size_t cols)
{
    // Figure out which cucumbers that can move to the right (i.e. towards the
    // most-significant bit of the line). For instance: if the occupied mask,
    // where both right-moving and down-moving cucumbers are considered, for a
    // row is:
    //
    //     m = 0011101111100100,
    //
    // then we want to isolate the highest-set bits in each contiguous set of
    // bits, taking into account wrap-around. Rotating right by 1 and masking
    // that off yields:
    //
    //     m              = 1011101111100101
    //     m >>> 1        = 1101110111110010
    //     m & ~(m >>> 1) = 0010001000000101
    //
    // which is exactly the mask of cucumbers that can move right. Actually
    // moving is then just a matter of removing bits using that mask and adding
    // them back to their new position (i.e. rotated left by 1).

    uint256 moved = 0;

    for (size_t i = 0; i < r.size(); i++) {
        const uint256 occupied = r[i] | d[i];
        const uint256 can_move = occupied & ~occupied.rotate_right1(cols) & r[i];
        r[i] ^= can_move;
        r[i] |= can_move.rotate_left1(cols);
        moved |= can_move;
    }

    return moved;
}

static bool
step_d(std::span<const uint256> r, std::span<uint256> d, std::span<uint256> pending_moves)
{
    DEBUG_ASSERT(d.size() == pending_moves.size());

    for (size_t i = 0; i + 1 < d.size(); i++)
        pending_moves[i] = d[i] & ~(r[i + 1] | d[i + 1]);
    pending_moves[d.size() - 1] = d[d.size() - 1] & ~(r[0] | d[0]);

    uint256 moved = 0;
    auto execute_move = [&](size_t from, size_t to) {
        moved |= pending_moves[from];
        d[from] &= ~pending_moves[from];
        d[to] |= pending_moves[from];
    };
    for (size_t i = 0; i + 1 < d.size(); i++)
        execute_move(i, i + 1);
    execute_move(d.size() - 1, 0);

    return moved;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::vector<uint256> r(lines.size());
    std::vector<uint256> d(lines.size());
    std::vector<uint256> d_tmp(lines.size());

    for (size_t i = 0; i < lines.size(); i++) {
        ASSERT(lines[0].size() == lines[i].size());
        for (size_t j = 0; j < lines[i].size(); j++) {
            ASSERT(lines[i].size() < 255);
            char c = lines[i][j];
            if (c == '>')
                r[i].set_bit(j);
            if (c == 'v')
                d[i].set_bit(j);
        }
    }

    const size_t cols = lines[0].size();

    size_t round = 0;
    while (true) {
        bool r_moved = step_r(r, d, cols);
        bool d_moved = step_d(r, d, d_tmp);
        if (!r_moved && !d_moved)
            break;
        round++;
    }

    fmt::print("{}\n", round + 1);
}

}
