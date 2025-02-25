#include "assembunny.h"
#include "common.h"

namespace aoc_2016_23 {

void run(std::string_view buf)
{
    auto prog = assemble(buf);
    optimize(prog);
    auto prog2 = prog;
    fmt::print("{}\n", run_program(prog, {7, 0, 0, 0}));
    fmt::print("{}\n", run_program(prog2, {12, 0, 0, 0}));
}

}
