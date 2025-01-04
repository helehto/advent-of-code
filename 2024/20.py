#!/usr/bin/env python3
import networkx as nx
import numpy as np
import sys


def neighbors(grid, i, j):
    if i > 0:
        yield (i - 1, j)
    if j > 0:
        yield (i, j - 1)
    if i + 1 < grid.shape[0]:
        yield (i + 1, j)
    if j + 1 < grid.shape[1]:
        yield (i, j + 1)


def main():
    grid = np.array([list(s.strip()) for s in sys.stdin.readlines()])
    bound = 2 if grid.shape[0] < 20 else 100

    G = nx.DiGraph()
    for u in np.ndindex(grid.shape):
        if grid[*u] != "#":
            G.add_node(tuple(u))
            for v in neighbors(grid, *u):
                if grid[*v] != "#":
                    G.add_edge(u, v)

    end = tuple(map(int, np.argwhere(grid == "E")[0]))
    dist = nx.single_source_dijkstra(G, end)[0]

    def solve(n):
        result = 0
        for u in sorted(G):
            for di in range(-n, n + 1):
                for dj in range(-n, n + 1):
                    d = abs(di) + abs(dj)
                    if d <= n and (v := (u[0] + di, u[1] + dj)) in G:
                        result += dist[u] - dist[v] - d >= bound
        return result

    print(solve(2))
    print(solve(20))


if __name__ == "__main__":
    main()
