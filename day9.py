#!/usr/bin/env python3
import sys
import numpy as np
lines = sys.stdin.read().strip().split()

chart = np.array([list(map(int,list(x))) for x in lines])


def neighbors(chart, i, j):
    return [(ni, nj) for ni, nj in ((i-1,j), (i,j+1), (i+1,j), (i, j-1))
            if 0 <= ni < chart.shape[0] and 0 <= nj < chart.shape[1]]


risk_sum = 0
for i in range(chart.shape[0]):
    for j in range(chart.shape[1]):
        ne = [chart[ni, nj] for ni, nj in neighbors(chart, i, j)]
        if chart[i, j] < min(ne):
            risk_sum += chart[i, j] + 1
print(risk_sum)


basins = np.array(chart)
basins[:] = -1
next_basin = 0
unvisited = set((i, j) for i in range(chart.shape[0]) for j in range(chart.shape[1])
                if chart[i, j] != 9)

while unvisited:
    i, j = next(iter(unvisited))

    basin = next_basin
    visited = []

    # Wander to a lower point.
    while True:
        if (i, j) not in unvisited:
            basin = basins[i, j]
            break
        unvisited.remove((i, j))

        # -2 for the currently visited line.
        basins[i, j] = -2
        visited.append((i, j))

        for ni, nj in neighbors(chart, i, j):
            if chart[ni, nj] < chart[i, j] and basins[ni, nj] != -2 and chart[ni, nj] != 9:
                i, j = ni, nj
                break
        else:
            # This is a low point or we have visited all neighbors.
            break

    for i, j in visited:
        basins[i, j] = basin

    if basin == next_basin:
        next_basin += 1


sizes = sorted((sum((basins == b).flatten()) for b in range(next_basin)), reverse=True)
print(sizes[0] * sizes[1] * sizes[2])
