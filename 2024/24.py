#!/usr/bin/env python3
import functools
import itertools
from itertools import permutations, product
import networkx as nx
import numpy as np
import re
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from typing import *
import z3


def evaluate(sections):
    s = z3.Solver()

    z3vars = {}
    for line in sections[0].splitlines():
        a, b = line.split(": ")
        v = z3.BitVec(a, 64)
        s.add(v == int(b))
        z3vars[a] = v

    for line in sections[1].splitlines():
        a, op, b, _, c = line.split()

        if (v := z3vars.get(a)) is None:
            z3vars[a] = (v := z3.BitVec(a, 64))
        if (v := z3vars.get(b)) is None:
            z3vars[b] = (v := z3.BitVec(b, 64))
        if (v := z3vars.get(c)) is None:
            z3vars[c] = (v := z3.BitVec(c, 64))

        if op == "XOR":
            s.add(z3vars[a] ^ z3vars[b] == z3vars[c])
        elif op == "OR":
            s.add(z3vars[a] | z3vars[b] == z3vars[c])
        elif op == "AND":
            s.add(z3vars[a] & z3vars[b] == z3vars[c])

    s.check()

    def getval(prefix):
        bits = []
        for var in sorted(z3vars.keys(), reverse=True):
            if var.startswith(prefix):
                bits.append(str(s.model()[z3vars[var]].as_long()))
        return int("".join(bits), 2)

    return (getval("x"), getval("y"), getval("z"))


def part1(sections):
    return evaluate(sections)[2]


def part2(sections):
    x, y, z = evaluate(sections)
    print(f"{x:064b}")
    print(f"{y:064b}")
    print("--")
    print(f"sum: {x+y:064b}")
    print(f"out: {z:064b}")
    markers = []
    for i in range(64):
        markers.append(" " if (x + y) & (1 << i) == z & (1 << i) else "^")
    print("     " + "".join(reversed(markers)))

    with open("d24.dot", "w") as f:
        print("digraph {", file=f)

        nodes = [x.split(": ")[0] for x in sections[0].splitlines()]
        for node in nodes:
            print(f"{node};", file=f)

        bad_outputs = [
            f"z{i:02}" for i in range(64) if (x + y) & (1 << i) != z & (1 << i)
        ]
        for node in bad_outputs:
            print(f"{node} [color=red];", file=f)

        for line in sections[1].splitlines():
            a, op, b, _, c = line.split()
            opnames = {"XOR": "^", "OR": "|", "AND": "&amp;"}
            print(f'{a} -> {c} [label="{opnames[op]}"];', file=f)
            print(f'{b} -> {c} [label="{opnames[op]}"];', file=f)

        print("}", file=f)

    print("(Part 2 was solved by staring angrily at the graph generated in d24.dot.)")


def main():
    sections = sys.stdin.read().split("\n\n")
    print(part1(sections))
    part2(sections)


if __name__ == "__main__":
    main()
