#pragma once

#include "common.h"

enum Instruction {
    instr_addr,
    instr_addi,
    instr_mulr,
    instr_muli,
    instr_banr,
    instr_bani,
    instr_borr,
    instr_bori,
    instr_setr,
    instr_seti,
    instr_gtir,
    instr_gtri,
    instr_gtrr,
    instr_eqir,
    instr_eqri,
    instr_eqrr,
    num_opcodes,
};

inline void execute(std::span<int> regs, const std::array<int, 4> &instr)
{
    auto rr = [&](auto &&f) { regs[instr[3]] = f(regs[instr[1]], regs[instr[2]]); };
    auto ri = [&](auto &&f) { regs[instr[3]] = f(regs[instr[1]], instr[2]); };
    auto ir = [&](auto &&f) { regs[instr[3]] = f(instr[1], regs[instr[2]]); };

    switch (instr[0]) {
    case instr_addr:
        rr(λab(a + b));
        break;
    case instr_addi:
        ri(λab(a + b));
        break;
    case instr_mulr:
        rr(λab(a * b));
        break;
    case instr_muli:
        ri(λab(a * b));
        break;
    case instr_banr:
        rr(λab(a & b));
        break;
    case instr_bani:
        ri(λab(a & b));
        break;
    case instr_borr:
        rr(λab(a | b));
        break;
    case instr_bori:
        ri(λab(a | b));
        break;
    case instr_setr:
        regs[instr[3]] = regs[instr[1]];
        break;
    case instr_seti:
        regs[instr[3]] = instr[1];
        break;
    case instr_gtir:
        ir(λab(a > b));
        break;
    case instr_gtri:
        ri(λab(a > b));
        break;
    case instr_gtrr:
        rr(λab(a > b));
        break;
    case instr_eqir:
        ir(λab(a == b));
        break;
    case instr_eqri:
        ri(λab(a == b));
        break;
    case instr_eqrr:
        rr(λab(a == b));
        break;
    }
}
