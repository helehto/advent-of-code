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
    def search1(upper_bound):
        s = z3.Solver()
        a = [z3.BitVec(f"a{i}", 64) for i in range(len(prog) + 1)]
        for i, _ in enumerate(prog):
            t = (a[i] & 7) ^ 5
            s.add(((t ^ (a[i] >> t)) & 7) == prog[i] ^ 6)
            s.add(a[i + 1] == a[i] >> 3)

        s.add(a[0] <= upper_bound)
        s.add(a[-1] == 0)

        s.check()
        try:
            a = s.model()[a[0]].as_long()
            return a
        except z3.z3types.Z3Exception:
            return None

    lo = 0
    hi = 2**63 - 1
    while lo + 1 < hi:
        mid = (hi + lo) // 2
        if (m := search1(mid)) is not None:
            result = m
            hi = mid
        else:
            lo = mid

    return result


def main():
    a, b, c, *prog = map(int, re.findall(r"\d+", sys.stdin.read()))
    print(",".join(map(str, run_program([a, b, c], prog))))
    print(search(prog))


if __name__ == "__main__":
    main()
