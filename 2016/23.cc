#include "assembunny.h"
#include "common.h"

namespace aoc_2016_23 {

void run(std::string_view buf, aoc::Answer &answer)
{
    auto prog = assemble(buf);
    optimize(prog);
    auto prog2 = prog;
    answer.add(run_program(prog, {7, 0, 0, 0}));
    answer.add(run_program(prog2, {12, 0, 0, 0}));
}
AOC_REGISTER_SOLVER(2016, 23, run);

}
