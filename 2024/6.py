#!/usr/bin/env python3
import sys


def walk(grid, p):
    d = (-1, 0)
    visited = {(p, d)}
    while True:
        q = (p[0] + d[0], p[1] + d[1])
        if q[0] < 0 or q[1] < 0:
            break

        try:
            while grid[q[0]][q[1]] == "#":
                d = (d[1], -d[0])
                q = (p[0] + d[0], p[1] + d[1])
        except IndexError:
            break

        if (q, d) in visited:
            return None
        visited.add((q, d))
        p = q

    return set(p for p, _ in visited)


def main():
    grid = []
    for l in sys.stdin.readlines():
        grid.append(list(l.strip()))

    p = next(
        (i, j) for i, line in enumerate(grid) for j, c in enumerate(line) if c == "^"
    )
    print(len(walk(grid, p)))

    nloops = 0
    for i, row in enumerate(grid):
        for j, c in enumerate(row):
            if c in "^.":
                grid[i][j] = "#"
                nloops += walk(grid, p) is None
                grid[i][j] = c
    print(nloops)


if __name__ == "__main__":
    main()
