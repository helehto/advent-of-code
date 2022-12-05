#!/usr/bin/env python3
import re
import sys


def main():
    cratelines, moves = sys.stdin.read().split("\n\n")

    cratelines = cratelines.split("\n")
    moves = [s for s in moves.split("\n") if s != ""]

    def parse_crates():
        crates = [list() for _ in range(30)]
        for line in cratelines:
            if line[1] == "1":
                break
            for i in range(1, len(line), 4):
                if line[i] != " ":
                    crates[(i + 3) // 4].append(line[i])
        return crates

    # part 1
    crates = parse_crates()
    for move in moves:
        n, f, t = map(int, re.findall("(\d+)", move))
        crates[t] = list(reversed(crates[f][:n])) + crates[t]
        crates[f] = crates[f][n:]
    print("".join([x[0] for x in crates if x]))

    # part 2
    crates = parse_crates()
    for move in moves:
        n, f, t = map(int, re.findall("(\d+)", move))
        crates[t] = crates[f][:n] + crates[t]
        crates[f] = crates[f][n:]
    print("".join([x[0] for x in crates if x]))


if __name__ == "__main__":
    main()
