#include "common.h"
#include "inplace_vector.h"

namespace aoc_2017_25 {

struct State {
    std::array<ssize_t, 2> write;
    std::array<ssize_t, 2> move;
    std::array<ssize_t, 2> transition;
};

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto [n] = find_numbers_n<int, 1>(lines[1]);

    std::vector<State> states;

    for (size_t i = 4; i < lines.size(); i += 10) {
        State s;
        s.write = {
            find_numbers_n<int, 1>(lines[i + 1])[0],
            find_numbers_n<int, 1>(lines[i + 5])[0],
        };
        s.move = {
            lines[i + 2].ends_with("left.") ? -1 : 1,
            lines[i + 6].ends_with("left.") ? -1 : 1,
        };
        s.transition = {
            lines[i + 3][lines[i + 3].size() - 2] - 'A',
            lines[i + 7][lines[i + 7].size() - 2] - 'A',
        };
        states.push_back(s);
    }

    inplace_vector<uint8_t, 100'000> tape(100'000);
    uint8_t *head = tape.data() + tape.size() / 2;
    for (int i = 0, s = 0; i < n; ++i) {
        const ssize_t sym = *head;
        *head = states[s].write[sym];
        head += states[s].move[sym];
        s = states[s].transition[sym];
    }

    fmt::print("{}\n", std::ranges::count(tape, 1));
}

}
