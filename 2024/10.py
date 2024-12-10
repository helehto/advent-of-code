#!/usr/bin/env python3
import numpy as np
import sys


def trailhead_score(grid, head, allow_revisit):
    queue = [head]
    visited = set()

    score = 0
    while queue:
        u = queue.pop(0)
        if not allow_revisit and tuple(u) in visited:
            continue
        visited.add(tuple(u))
        score += grid[*u] == 9

        for d in np.array(((-1, 0), (1, 0), (0, -1), (0, 1))):
            v = u + d
            if (v >= 0).all() and (v < grid.shape).all() and grid[*v] - grid[*u] == 1:
                queue.append(v)

    return score


def main():
    grid = []
    for line in sys.stdin.readlines():
        grid.append(list(map(int,line.strip())))
    grid = np.array(grid)

    s1,s2 = 0,0
    for u in np.ndindex(grid.shape):
        if grid[*u] == 0:
            s1 += trailhead_score(grid, u, False)
            s2 += trailhead_score(grid, u, True)
    print(s1)
    print(s2)

if __name__ == "__main__":
    main()
