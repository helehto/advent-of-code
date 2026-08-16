#include "assembunny.h"
#include "common.h"

namespace aoc_2016_12 {

void run(std::string_view buf, aoc::Answer &answer)
{
    auto prog = assemble(buf);
    optimize(prog);
    answer.add(run_program(prog, {0, 0, 0, 0}));
    answer.add(run_program(prog, {0, 0, 1, 0}));
}
AOC_REGISTER_SOLVER(2016, 12, run);

}
