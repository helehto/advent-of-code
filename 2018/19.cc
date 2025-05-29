#include "common.h"
#include "vm.h"

namespace aoc_2018_19 {

constexpr int get_mystery_number(int ip_reg,
                                 std::span<const std::string_view> lines,
                                 std::array<int, 6> regs)
{

    size_t ip = 0;
    while (true) {
        ASSERT(ip < lines.size());
        if (ip == 1)
            return regs[4];

        Instruction opcode;
        regs[ip_reg] = ip;
        std::string_view line = lines[ip];

        if (line.starts_with("addr"))
            opcode = instr_addr;
        else if (line.starts_with("addi"))
            opcode = instr_addi;
        else if (line.starts_with("mulr"))
            opcode = instr_mulr;
        else if (line.starts_with("muli"))
            opcode = instr_muli;
        else if (line.starts_with("banr"))
            opcode = instr_banr;
        else if (line.starts_with("bani"))
            opcode = instr_bani;
        else if (line.starts_with("borr"))
            opcode = instr_borr;
        else if (line.starts_with("bori"))
            opcode = instr_bori;
        else if (line.starts_with("setr"))
            opcode = instr_setr;
        else if (line.starts_with("seti"))
            opcode = instr_seti;
        else if (line.starts_with("gtir"))
            opcode = instr_gtir;
        else if (line.starts_with("gtri"))
            opcode = instr_gtri;
        else if (line.starts_with("gtrr"))
            opcode = instr_gtrr;
        else if (line.starts_with("eqir"))
            opcode = instr_eqir;
        else if (line.starts_with("eqri"))
            opcode = instr_eqri;
        else if (line.starts_with("eqrr"))
            opcode = instr_eqrr;
        else
            ASSERT(false);

        const auto [a, b, c] = find_numbers_n<int, 3>(lines[regs[ip_reg]]);
        std::array<int, 4> instr{opcode, a, b, c};
        execute(regs, instr);

        ip = regs[ip_reg] + 1;
    }
}

/// Sum of divisors of N.
constexpr int sigma(int n)
{
    int result = n + 1;
    const auto bound = static_cast<int>(std::ceil(std::sqrt(n)));

    ASSERT(n % 2 == 1);
    for (int k = 3; k <= bound; k += 2) {
        if (n % k == 0)
            result += k + n / k;
    }

    return result;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    ASSERT(lines[0].starts_with("#ip"));

    const auto [ip_reg] = find_numbers_n<int, 1>(lines[0]);
    lines.erase(lines.begin());
    ASSERT(ip_reg >= 0 && ip_reg < 6);

    auto solve = [&](int a) {
        std::array<int, 6> regs{a, 0, 0, 0, 0, 0};
        return sigma(get_mystery_number(ip_reg, lines, regs));
    };

    fmt::print("{}\n", solve(0));
    fmt::print("{}\n", solve(1));
}

}
