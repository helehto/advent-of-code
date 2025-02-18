#include "common.h"

namespace aoc_2022_10 {

struct Instruction {
    int due;
    int addend;
};

void run(std::string_view buf)
{
    int X = 1;
    int cycle = 1;
    int signal_strength = 0;
    std::string crt;
    Instruction pending{-1, 0};

    auto lines = split_lines(buf);
    for (size_t i = 0; i < lines.size();) {
        // Start of the cycle. If nothing is executing, fetch an instruction.
        if (pending.due < 0) {
            auto &line = lines[i++];
            if (line == "noop") {
                pending = {cycle, 0};
            } else {
                int addend = 0;
                std::from_chars(line.data() + 5, line.data() + line.size(), addend);
                pending = {cycle + 1, addend};
            }
        }

        // Get signal strength for part 1:
        if (cycle % 40 == 20)
            signal_strength += cycle * X;

        // Draw to CRT for part 2:
        const int col = (cycle - 1) % 40;
        if (!crt.empty() && col == 0)
            crt += '\n';
        crt += " #"[X - 1 <= col && col <= X + 1];

        // End of the cycle.
        if (pending.due == cycle) {
            // Current instruction is due; finish it.
            X += pending.addend;
            pending.due = -1;
        }

        cycle++;
    }

    fmt::print("{}\n", signal_strength);
    fmt::print("{}\n", crt);
}

}
