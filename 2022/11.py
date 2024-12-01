#!/usr/bin/env python3
import re
import sys
from dataclasses import dataclass, field
from typing import *


@dataclass
class Monkey:
    items: Any
    op: str
    div: int
    targets: Tuple[int, int]
    inspections: int = field(default=0)

    @staticmethod
    def parse(descr):
        descr = descr.split("\n")
        items = list(map(int, re.findall(r"(\d+)", descr[1])))
        op = descr[2].split(" = ")[1].strip().split()
        div = int(re.findall(r"(\d+)", descr[3])[0])
        ift = int(re.findall(r"(\d+)", descr[4])[0])
        iff = int(re.findall(r"(\d+)", descr[5])[0])
        return Monkey(items, op, div, (ift, iff))


def part1(sections):
    monkeys = list(map(Monkey.parse, sections))

    for _ in range(20):
        for m in monkeys:
            for w in m.items:
                match m.op:
                    case ["old", "*", "old"]:
                        nw = w * w
                    case ["old", "*", x]:
                        nw = w * int(x)
                    case ["old", "+", x]:
                        nw = w + int(x)

                nw //= 3
                target = m.targets[nw % m.div != 0]
                monkeys[target].items.append(nw)

            m.inspections += len(m.items)
            m.items = []

    q = sorted(m.inspections for m in monkeys)
    return q[-1] * q[-2]


def part2(sections):
    monkeys = list(map(Monkey.parse, sections))

    # Expand each item into a list, where index i in the list encodes the worry
    # of the item modulo monkey i's divisor. By maintaining this representation
    # exponential blowup is avoided.
    for m in monkeys:
        m.items = [[it % m2.div for m2 in monkeys] for it in m.items]

    for _ in range(10000):
        for i, m in enumerate(monkeys):
            for w in m.items:
                match m.op:
                    case ["old", "*", "old"]:
                        nw = [(e * e) % monkeys[j].div for j, e in enumerate(w)]
                    case ["old", "*", x]:
                        nw = [(e * int(x)) % monkeys[j].div for j, e in enumerate(w)]
                    case ["old", "+", x]:
                        nw = [(e + int(x)) % monkeys[j].div for j, e in enumerate(w)]

                target = m.targets[nw[i] != 0]
                monkeys[target].items.append(nw)

            m.inspections += len(m.items)
            m.items = []

    q = sorted(m.inspections for m in monkeys)
    return q[-1] * q[-2]


def main():
    sections = sys.stdin.read().strip().split("\n\n")
    print(part1(sections))
    print(part2(sections))


if __name__ == "__main__":
    main()
