#!/usr/bin/env python3
import itertools
import sys
import numpy as np
lines = sys.stdin.read().strip().split()


def neighbors(g, i, j):
    candidates = ((i-1,j-1), (i-1,j), (i-1,j+1),
                  (i,j-1), (i,j+1),
                  (i+1,j-1), (i+1,j), (i+1,j+1))

    return [(ni, nj) for ni, nj in candidates if
            0 <= ni < g.shape[0] and 0 <= nj < g.shape[1]]

grid = np.array([[int(c) for c in line] for line in lines])

flashes = 0
for step in itertools.count(1):
    grid[:] += 1

    flashed = set()
    queue = [c for c in np.ndindex(grid.shape) if grid[c] > 9]
    for c in queue:
        if c not in flashed:
            flashed.add(c)
            for n in neighbors(grid, *c):
                grid[n] += 1
                if grid[n] > 9:
                    queue.append(n)

    mask = grid > 9
    flashes += mask.sum()
    grid[mask] = 0

    if step == 100:
        # Part 1
        print(flashes)
    elif len(flashed) == grid.size:
        # Part 2
        print(step)
        break
