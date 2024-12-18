#!/usr/bin/env python3
import re
import sys
import z3


def run_program(reg, prog):
    ip, op, val = 0, 0, 0
    A, B, C = range(3)

    def combo_operand():
        return val if val < 4 else reg[val - 4]

    while ip < len(prog):
        op, val = prog[ip], prog[ip + 1]
        if op in (0, 6, 7):
            reg[{0: A, 6: B, 7: C}[op]] = reg[A] >> combo_operand()
        elif op in (1, 4):
            reg[B] ^= {1: val, 4: reg[C]}[op]
        elif op == 2:
            reg[B] = combo_operand() & 7
        elif op == 3:
            if reg[A] != 0:
                ip = val - 2
        else:
            yield combo_operand() & 7
        ip += 2


def search(prog):
    s = z3.Optimize()
    a_init = z3.BitVec("a", 64)

    a = a_init
    for val in prog:
        b = a & 7
        b ^= 5
        c = a >> b
        b ^= c
        b ^= 6
        s.add(b & 7 == val)
        a >>= 3

    s.minimize(a_init)
    s.check()
    return s.model()[a_init].as_long()


def main():
    a, b, c, *prog = map(int, re.findall(r"\d+", sys.stdin.read()))
    print(",".join(map(str, run_program([a, b, c], prog))))
    print(search(prog))


if __name__ == "__main__":
    main()
