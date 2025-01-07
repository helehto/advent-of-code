#include "common.h"

namespace aoc_2019_2 {

void run_program(std::vector<int> &prog)
{
    for (size_t i = 0; i < prog.size();) {
        int opcode = prog[i];
        if (opcode == 99) {
            break;
        } else if (opcode == 1) {
            prog[prog[i + 3]] = prog[prog[i + 1]] + prog[prog[i + 2]];
            i += 4;
        } else if (opcode == 2) {
            prog[prog[i + 3]] = prog[prog[i + 1]] * prog[prog[i + 2]];
            i += 4;
        } else {
            ASSERT_MSG(false, "Unknown opcode {}", opcode);
        }
    }
}

void run(FILE *f)
{
    auto buf = slurp(f);
    auto orig_prog = find_numbers<int>(buf);
    std::vector<int> prog;

    prog = orig_prog;
    prog[1] = 12;
    prog[2] = 5;
    run_program(prog);
    fmt::print("{}\n", prog.front());

    for (int verb = 1; verb < 100; ++verb) {
        for (int noun = 1; noun < 100; ++noun) {
            prog = orig_prog;
            prog[1] = noun;
            prog[2] = verb;
            run_program(prog);
            if (prog.front() == 19690720) {
                fmt::print("{}\n", 100 * noun + verb);
                break;
            }
        }
    }
}

}
