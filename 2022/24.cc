#include "common.h"
#include "uint256.h"

namespace aoc_2022_24 {

struct State {
    std::vector<uint256> n; // ^
    std::vector<uint256> s; // v
    std::vector<uint256> w; // <
    std::vector<uint256> e; // >
    std::vector<uint256> elves;
    std::vector<uint256> elves_tmp;
    uint256 mask; // mask for in-bounds columns
    size_t rows;
    size_t cols;
};

static State parse_input(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines);

    State state;
    state.n.resize(grid.rows - 2);
    state.s.resize(grid.rows - 2);
    state.w.resize(grid.rows - 2);
    state.e.resize(grid.rows - 2);
    state.rows = grid.rows - 2;
    state.cols = grid.cols - 2;

    for (size_t i = 1; i < grid.rows - 1; ++i) {
        for (size_t j = 1; j < grid.cols - 1; ++j) {
            if (grid(i, j) == '^')
                state.n[i - 1].set_bit(j - 1);
            else if (grid(i, j) == 'v')
                state.s[i - 1].set_bit(j - 1);
            else if (grid(i, j) == '<')
                state.w[i - 1].set_bit(j - 1);
            else if (grid(i, j) == '>')
                state.e[i - 1].set_bit(j - 1);
        }
    }

    state.mask = uint256::ones(state.cols);
    state.elves.resize(state.rows);
    state.elves_tmp.resize(state.rows);
    return state;
}

static void step_storms(State &state)
{
    for (size_t i = 0; i < state.rows; ++i) {
        state.w[i] = state.w[i].rotate_right1(state.cols);
        state.e[i] = state.e[i].rotate_left1(state.cols);
    }

    std::ranges::rotate(state.n, state.n.begin() + 1);
    std::ranges::rotate(state.s, state.s.begin() + state.s.size() - 1);
}

static void step_elves(State &state)
{
    const size_t n = state.rows;
    auto &prev = state.elves;
    auto &next = state.elves_tmp;

    auto blocked = [&](size_t i) {
        return state.n[i] | state.s[i] | state.w[i] | state.e[i];
    };
    auto hstep = [&](size_t i) {
        return prev[i].shift_right1() | (prev[i].shift_left1() & state.mask);
    };
    auto vstep = [&](size_t i) { return prev[i - 1] | prev[i + 1]; };

    // Through the magic of quantum elves™, compute all possible positions of
    // elves for this time step in a single pass over the input, an entire line
    // at a time. As soon as any elf has reached the goal, we have found the
    // shortest path.
    next[0] = (prev[1] | hstep(0)) & ~blocked(0);
    next[n - 1] = (prev[n - 2] | hstep(n - 1)) & ~blocked(n - 1);
    for (size_t i = 1; i < n - 1; ++i)
        next[i] = (prev[i] | vstep(i) | hstep(i)) & ~blocked(i);

    std::swap(prev, next);
}

static int walk(State &state, size_t x0, size_t y0, size_t x1, size_t y1)
{
    state.elves.assign(state.rows, uint256(0));

    size_t t = 0;
    for (; !state.elves[y1].test_bit(x1); ++t) {
        step_storms(state);
        step_elves(state);
        if (!(state.n[y0] | state.s[y0] | state.w[y0] | state.e[y0]).test_bit(x0))
            state.elves[y0].set_bit(x0);
    }

    // Account for the last step from inside the grid to the goal.
    t++;
    step_storms(state);

    return t;
}

void run(std::string_view buf)
{
    auto state = parse_input(buf);
    auto t1 = walk(state, 0, 0, state.cols - 1, state.rows - 1);
    auto t2 = walk(state, state.cols - 1, state.rows - 1, 0, 0);
    auto t3 = walk(state, 0, 0, state.cols - 1, state.rows - 1);
    fmt::print("{}\n", t1);
    fmt::print("{}\n", t1 + t2 + t3);
}

}
