#!/usr/bin/env python3
import re
import sys
import z3


def build_model(lines, part):
    names = [re.sub(":.*", "", s) for s in lines]
    v = {x: z3.Int(x) for x in names}
    solver = z3.Solver()

    for line in lines:
        match line.replace(":", "").split():
            case [x, n]:
                if part == 1 or x != "humn":
                    solver.add(v[x] == n)
            case [x, a, op, b]:
                if part == 2 and x == "root":
                    solver.add(v[a] == v[b])
                elif op == "+":
                    solver.add(v[x] == v[a] + v[b])
                elif op == "-":
                    solver.add(v[x] == v[a] - v[b])
                elif op == "*":
                    solver.add(v[x] == v[a] * v[b])
                elif op == "/":
                    solver.add(v[x] * v[b] == v[a])

    solver.check()
    target = "root" if part == 1 else "humn"
    return solver.model()[v[target]]


def main():
    lines = [s.strip() for s in sys.stdin.readlines()]
    print(build_model(lines, 1))
    print(build_model(lines, 2))


if __name__ == "__main__":
    main()
