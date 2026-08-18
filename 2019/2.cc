#include <aoc/base.h>
#include <aoc/macros.h>
#include <aoc/small_vector.h>
#include <aoc/string.h>
#include <cstdint>
#include <string_view>

namespace aoc_2019_2 {

void run(std::string_view buf, aoc::Answer &answer)
{
    small_vector<uint32_t, 256> prog;
    find_numbers(buf, prog);

    auto run_program = [&](uint32_t noun, uint32_t verb) {
        auto mem = prog;
        mem[1] = noun;
        mem[2] = verb;
        for (int pc = 0;; pc += 4) {
            if (mem[pc] == 1)
                mem[mem[pc + 3]] = mem[mem[pc + 1]] + mem[mem[pc + 2]];
            else if (mem[pc] == 2)
                mem[mem[pc + 3]] = mem[mem[pc + 1]] * mem[mem[pc + 2]];
            else if (mem[pc] == 99)
                return mem[0];
            else
                ASSERT_MSG(false, "Invalid opcode {} at pc={}", mem[pc], pc);
        }
    };

    // The intcode program computes `noun * c₁ + verb + c₂` for some constants
    // c₁ and c₂ that depend on the specific input program.
    //
    // Running with noun=0, verb=0 yields c₂.
    // Running with noun=1, verb=0 yields c₁ + c₂, from which we get c₁.
    auto c2 = run_program(0, 0);
    auto c1 = run_program(1, 0) - c2;
    answer.add(12 * c1 + 2 + c2);

    // Given c₁ ≥ 100, 0 ≤ noun < 100 and 0 ≤ verb < 100, the only solution to
    // `noun * c₁ + verb + c₂ = 19690720` is this:
    const auto noun = (19690720 - c2) / c1;
    const auto verb = (19690720 - c2) % c1;
    ASSERT(noun < 100);
    ASSERT(verb < 100);
    answer.add(100 * noun + verb);
}
AOC_REGISTER_SOLVER(2019, 2, run);

}
