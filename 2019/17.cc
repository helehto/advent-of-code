#include "common.h"
#include "dense_set.h"
#include "intcode.h"

namespace aoc_2019_17 {

using VM = IntcodeVM<SplitMemory<int64_t>>;

enum { L = -1, R = -2 };
struct Move {
    int8_t v;
    bool operator==(const Move &) const = default;
    size_t ascii_length() const { return v < 10 ? 1 : 2; }
};

static size_t sequence_ascii_length(std::span<const Move> a)
{
    size_t len = 0;
    for (const Move m : a)
        len += m.ascii_length();
    return len + a.size() - 1;
}

static bool search(std::span<const Move> curr,
                   std::string &main_routine,
                   std::array<std::span<const Move>, 3> &functions)
{
    const size_t orig_main_routine_len = main_routine.size();

    // Remove any prefixes that form functions.
    bool changed;
    do {
        changed = false;
        for (size_t i = 0; i < functions.size(); i++) {
            auto &f = functions[i];
            if (f.empty())
                continue;

            while (curr.size() >= f.size() &&
                   std::ranges::equal(curr.subspan(0, f.size()), f)) {
                curr = curr.subspan(f.size());
                main_routine += 'A' + i;
                changed = true;
            }
        }
    } while (changed);

    if (curr.empty())
        return true;

    // If we have any free slots for functions, recurse by trying to define a
    // function as successively longer prefixes of the current sequence.
    for (size_t s = 0; s < functions.size(); s++) {
        if (!functions[s].empty())
            continue;

        for (size_t i = 1; i < curr.size(); i++) {
            if (sequence_ascii_length(curr.subspan(0, i)) <= 20) {
                functions[s] = curr.subspan(0, i);
                main_routine.push_back('A' + s);
                if (search(curr.subspan(i), main_routine, functions))
                    return true;
                main_routine.pop_back();
                functions[s] = {};
            }
        }
    }

    main_routine.resize(orig_main_routine_len);
    return false;
}

static std::vector<Move> walk_scaffold(const Matrix<char> &g)
{
    // Find our starting location.
    Vec2i p{};
    for (auto q : g.ndindex<int>()) {
        if (g(q) == '^') {
            p = q;
            break;
        }
    }

    std::vector<Move> result;
    result.push_back(Move{L});

    Vec2i d = {-1, 0};
    while (true) {
        int steps = 0;
        for (;; steps++) {
            auto next = p + d;
            if (!g.in_bounds(next) || g(next) != '#')
                break;
            p = next;
        }
        result.emplace_back(steps);

        const Vec2i dl = d.ccw();
        if (g.in_bounds(p + dl) && g(p + dl) == '#') {
            result.emplace_back(L);
            d = dl;
            continue;
        }

        const Vec2i dr = d.cw();
        if (g.in_bounds(p + dr) && g(p + dr) == '#') {
            result.emplace_back(R);
            d = dr;
            continue;
        }

        return result;
    }
}

static std::vector<VM::value_type>
construct_input(std::string_view main_routine,
                std::span<const std::span<const Move>> functions)
{
    std::vector<VM::value_type> result;
    result.reserve(150);

    for (size_t i = 0; i < main_routine.size(); i++) {
        if (i)
            result.push_back(',');
        result.push_back(main_routine[i]);
    }
    result.push_back('\n');
    for (std::span<const Move> f : functions) {
        for (size_t i = 0; i < f.size(); i++) {
            if (i)
                result.push_back(',');
            const int m = f[i].v;
            if (m == L) {
                result.push_back('L');
            } else if (m == R) {
                result.push_back('R');
            } else {
                for (char c : std::to_string(m))
                    result.push_back(c);
            }
        }
        result.push_back('\n');
    }
    result.push_back('n');
    result.push_back('\n');

    return result;
}

static int part1(const Matrix<char> &g)
{
    int alignment_sum = 0;
    for (auto p : g.ndindex()) {
        if (g(p) == '#') {
            int n = 0;
            for (auto q : neighbors4(g, p))
                n += g(q) == '#';
            if (n == 4)
                alignment_sum += p.x * p.y;
        }
    }
    return alignment_sum;
}

static int part2(const Matrix<char> &g, VM &vm, std::span<const int64_t> prog)
{
    // Generate the full sequence of moves needed to visit the entire scaffold.
    std::vector<Move> move_list = walk_scaffold(g);

    // Break the sequence of moves into a main routine and subordinate
    // functions.
    std::string main_routine;
    std::array<std::span<const Move>, 3> functions{};
    search(move_list, main_routine, functions);

    // Prepare the interpreter and feed it the correctly formatted input.
    vm.reset(prog);
    vm.mem.wr(0, 2);
    vm.input = construct_input(main_routine, functions);

    ASSERT(vm.run() == HaltReason::op99);
    return vm.output.back();
}

static Matrix<char> construct_grid(VM &vm, std::span<const int64_t> prog)
{
    vm.reset(prog);
    vm.run();

    const size_t cols = std::ranges::find(vm.output, '\n') - vm.output.begin();
    const size_t rows = std::ranges::count(vm.output, '\n') - 1;
    Matrix<char> g(rows, cols);

    size_t x = 0;
    size_t y = 0;
    for (char c : vm.output) {
        if (c == '\n') {
            y++;
            x = 0;
        } else {
            g(y, x) = c;
            x++;
        }
    }

    return g;
}

void run(std::string_view buf)
{
    auto prog = find_numbers<VM::value_type>(buf);
    VM vm;
    Matrix<char> g = construct_grid(vm, prog);
    fmt::print("{}\n", part1(g));
    fmt::print("{}\n", part2(g, vm, prog));
}

}
