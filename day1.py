#!/usr/bin/env python3
import sys


def main():
    chunks = sys.stdin.read().split("\n\n")
    elves = [list(map(int, x.split())) for x in chunks]
    cals = sorted(sum(x) for x in elves)
    print(cals[-1])
    print(sum(cals[-3:]))


if __name__ == "__main__":
    main()
