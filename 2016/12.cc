#include "assembunny.h"
#include "common.h"

namespace aoc_2016_12 {

void run(std::string_view buf)
{
    auto prog = assemble(buf);
    optimize(prog);
    fmt::print("{}\n", run_program(prog, {0, 0, 0, 0}));
    fmt::print("{}\n", run_program(prog, {0, 0, 1, 0}));
}

}
