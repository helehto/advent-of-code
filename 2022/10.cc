#include "common.h"
#include <charconv>
#include <fmt/core.h>
#include <string>
#include <vector>

struct Instruction {
    int due;
    int addend;
};

void run_2022_10(FILE *f)
{
    int X = 1;
    int cycle = 1;
    int signal_strength = 0;
    std::string crt;
    Instruction pending{-1, 0};
    std::vector<std::string> lines;

    std::string s;
    while (getline(f, s)) {
        if (!s.empty())
            lines.push_back(std::move(s));
    }

    size_t i = 0;
    while (i < lines.size()) {
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
