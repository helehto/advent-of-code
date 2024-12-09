#!/usr/bin/env python3
import itertools
import numpy as np
import sys


def main():
    grid = []
    for line in sys.stdin.readlines():
        grid.append(list(line.strip()))

    antennas = {}
    for i, line in enumerate(grid):
        for j, c in enumerate(line):
            if c not in ".#":
                antennas.setdefault(c, []).append(np.array((i, j)))

    h = len(grid)
    w = len(grid[0])

    antinodes1 = set()
    antinodes2 = set()
    for p in antennas.values():
        for p0, p1 in itertools.permutations(p, 2):
            for k in range(max(h, w)):
                a = p1 - k * (p1 - p0)
                if (0 <= a[0] < w) and (0 <= a[1] < h):
                    if k == 2:
                        antinodes1.add(tuple(a))
                    antinodes2.add(tuple(a))

                a = p0 + k * (p1 - p0)
                if (0 <= a[0] < w) and (0 <= a[1] < h):
                    if k == 2:
                        antinodes1.add(tuple(a))
                    antinodes2.add(tuple(a))

    print(len(antinodes1))
    print(len(antinodes2))


if __name__ == "__main__":
    main()
