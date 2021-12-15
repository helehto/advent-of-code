#!/usr/bin/env python3
import sys
import numpy as np
from collections import Counter
import heapq


def neighbors(weights, i, j):
    result = []
    if i > 0:
        result.append((i-1, j))
    if j > 0:
        result.append((i, j-1))
    if i + 1 < weights.shape[0]:
        result.append((i+1, j))
    if j + 1 < weights.shape[1]:
        result.append((i, j+1))
    return result


def dijkstra(weights):
    dist = {(0, 0): 0}
    visited = set()
    q = [(0, (0, 0))]
    queued = set((0, 0))
    inf = float('Inf')

    while q:
        cost, u = heapq.heappop(q)
        visited.add(u)
        queued.discard(u)

        for v in neighbors(weights, *u):
            if v not in visited:
                alt = cost + weights[v]
                d = dist.get(v, inf)
                if alt < d:
                    dist[v] = alt
                    d = alt
                if v not in queued:
                    heapq.heappush(q, (d, v))
                    queued.add(v)

    return dist[weights.shape[0] - 1, weights.shape[1] - 1]


def extend(grid, n=5):
    h, w = grid.shape
    result = np.zeros((h * n, w * n), dtype=np.int32)
    result[:h, :w] = grid

    def tile(w):
        m = w + 1
        m[m > 9] = 1
        return m

    for kw in range(w, n * w, w):
        result[:h, kw:kw+w] = tile(result[:h, kw-w:kw])

    for kh in range(h, n * h, h):
        result[kh:kh+h, :w] = tile(result[kh-h:kh, :w])

    for kh in range(h, n * h, h):
        for kw in range(w, n * w, w):
            result[kh:kh+h, kw:kw+w] = tile(result[kh-h:kh, kw:kw+w])

    return result


lines = sys.stdin.read().strip().split('\n')
weights = np.array([list(map(int,list(x))) for x in lines], dtype=np.int32)
print(dijkstra(weights))
print(dijkstra(extend(weights)))
