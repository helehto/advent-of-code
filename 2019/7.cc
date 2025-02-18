#include "common.h"
#include "intcode.h"

namespace aoc_2019_7 {

using VM = IntcodeVM<FlatMemory<int>>;

static int part1(std::array<VM, 5> &amplifiers, std::span<const VM::value_type> prog)
{
    std::array<int8_t, 5> perm;
    for (size_t i = 0; i < perm.size(); ++i)
        perm[i] = i;

    int max_thruster_value = INT_MIN;
    do {
        int last_output = 0;
        for (size_t i = 0; i < 5; i++) {
            amplifiers[i].reset(prog);
            amplifiers[i].run({perm[i], last_output});
            last_output = amplifiers[i].output[0];
        }
        max_thruster_value = std::max(max_thruster_value, last_output);
    } while (std::next_permutation(begin(perm), end(perm)));

    return max_thruster_value;
}

static int part2(std::array<VM, 5> &amplifiers, std::span<const VM::value_type> prog)
{
    std::array<int8_t, 5> perm;
    for (size_t i = 0; i < perm.size(); ++i)
        perm[i] = i + 5;

    int max_thruster_value = INT_MIN;
    do {
        for (size_t i = 0; i < 5; i++) {
            amplifiers[i].reset(prog);
            amplifiers[i].input.push_back(perm[i]);
        }
        int last_thruster_value = 0;
        int last_output = 0;
        for (size_t i = 0; i < 5; i = (i < 4) ? i + 1 : 0) {
            HaltReason reason = amplifiers[i].run({last_output});
            ASSERT(amplifiers[i].output.size() == 1);
            last_output = amplifiers[i].output[0];
            amplifiers[i].output.clear();
            if (i == 4) {
                last_thruster_value = last_output;
                if (reason == HaltReason::op99)
                    break;
            }
        }
        max_thruster_value = std::max(max_thruster_value, last_thruster_value);
    } while (std::next_permutation(begin(perm), end(perm)));

    return max_thruster_value;
}

void run(std::string_view buf)
{
    const auto prog = find_numbers<VM::value_type>(buf);
    std::array<VM, 5> amplifiers;
    fmt::print("{}\n", part1(amplifiers, prog));
    fmt::print("{}\n", part2(amplifiers, prog));
}

}
