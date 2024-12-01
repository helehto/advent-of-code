#!/usr/bin/env python3
import sys
import numpy as np


def neighbors(ar, i, j):
    n = ((i - 1, j), (i + 1, j), (i, j - 1), (i, j + 1))
    return [(y, x) for y, x in n if 0 <= y < ar.shape[0] and 0 <= x < ar.shape[1]]


def candidate_moves(ar, i, j):
    return [(y, x) for y, x in neighbors(ar, i, j) if ar[y, x] - ar[i, j] <= 1]


def solve(m, starts, end):
    q = list(np.ndindex(m.shape))
    dist = {**{x: 1e6 for x in q}, **{tuple(s): 0 for s in starts}}

    while q:
        # This runs fast enough without a heap.
        j = min(range(len(q)), key=lambda i: dist[q[i]])
        u = q.pop(j)

        for v in candidate_moves(m, *u):
            dist[v] = min(dist[v], dist[u] + 1)

    return dist[end]


def main():
    lines = [s.strip() for s in sys.stdin.readlines()]
    m = np.array([[int(ord(x)) for x in row] for row in lines])

    start = tuple(np.argwhere(m == ord("S"))[0])
    end = tuple(np.argwhere(m == ord("E"))[0])
    m[start] = ord("a")
    m[end] = ord("z")

    print(solve(m, [start], end))
    print(solve(m, np.argwhere(m == ord("a")).tolist(), end))


if __name__ == "__main__":
    main()
