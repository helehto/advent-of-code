#!/usr/bin/env python3
import numpy as np
import sys


def regions_of(g, seeds):
    visited = np.zeros(g.shape, dtype=bool)

    def neighbors(u):
        for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            v = (u[0] + dy, u[1] + dx)
            if 0 <= v[0] < g.shape[0] and 0 <= v[1] < g.shape[1]:
                yield v

    def dfs(p):
        q = [p]
        while q:
            u = q.pop()
            if not visited[*u]:
                visited[*u] = True
                yield tuple(u)
                q.extend(v for v in neighbors(u) if g[*u] == g[*v])

    for p in seeds:
        if not visited[*p]:
            yield tuple(dfs(p))


def fence_between(g, u, v):
    return not (0 <= v[0] < g.shape[0] and 0 <= v[1] < g.shape[1] and g[*u] == g[*v])


def region_perimeter(g, points):
    return sum(
        fence_between(g, u, (u[0] + dy, u[1] + dx))
        for u in points
        for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1))
    )


def region_sides(g, points):
    wall_masks = [np.zeros(g.shape, dtype=bool) for _ in range(4)]
    for u in points:
        for mask, (dy, dx) in zip(wall_masks, ((-1, 0), (1, 0), (0, -1), (0, 1))):
            mask[*u] = fence_between(g, u, (u[0] + dy, u[1] + dx))

    return sum(len(tuple(regions_of(mask, np.argwhere(mask)))) for mask in wall_masks)


def main():
    g = []
    for line in sys.stdin.readlines():
        g.append(list(line.strip()))
    g = np.array(g)

    s1, s2 = 0, 0
    ccs = tuple(regions_of(g, np.ndindex(g.shape)))
    print(sum(len(r) * region_perimeter(g, r) for r in ccs))
    print(sum(len(r) * region_sides(g, r) for r in ccs))


if __name__ == "__main__":
    main()
