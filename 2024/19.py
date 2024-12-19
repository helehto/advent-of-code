#!/usr/bin/env python3
import functools
import sys


@functools.cache
def solve1(s):
    return not s or any(s.startswith(d) and solve1(s[len(d) :]) for d in DESIGNS)


@functools.cache
def solve2(s):
    return sum(solve2(s[len(d) :]) for d in DESIGNS if s.startswith(d)) if s else 1


def main():
    global DESIGNS
    inp = sys.stdin.read().split("\n\n")
    DESIGNS, lines = tuple(inp[0].split(", ")), tuple(inp[1].splitlines())
    print(sum(map(solve1, lines)))
    print(sum(map(solve2, lines)))


if __name__ == "__main__":
    main()
