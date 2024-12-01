#!/usr/bin/env python3
import itertools
import sys


def replace(t, i, v):
    return t[:i] + (v,) + t[i + 1 :]


def sum_surface_area(cubes):
    occlusion = {}

    for c in cubes:
        for dim in (0, 1, 2):
            for db, d in ((0, -1), (1, 1)):
                n = replace(c, dim, c[dim] + d)
                if n in cubes:
                    occlusion[c] = occlusion.get(c, 0) | (1 << (2 * dim + db))

    return sum(6 - occlusion.get(c, 0).bit_count() for c in cubes)


def flood(cubes):
    xmin = min(c[0] for c in cubes) - 1
    xmax = max(c[0] for c in cubes) + 1
    ymin = min(c[1] for c in cubes) - 1
    ymax = max(c[1] for c in cubes) + 1
    zmin = min(c[2] for c in cubes) - 1
    zmax = max(c[2] for c in cubes) + 1

    occupied = set(cubes)

    def can_fill(c):
        # Can't fill out of bounds
        if not (xmin <= c[0] <= xmax and ymin <= c[1] <= ymax and zmin <= c[2] <= zmax):
            return False

        # Can only fill empty space, not lava cubes or water/steam
        return w not in occupied

    q = [(xmin, ymin, zmin)]
    while q:
        v = q.pop()
        occupied.add(v)

        for dim, d in itertools.product((0, 1, 2), (-1, 1)):
            w = replace(v, dim, v[dim] + d)
            if can_fill(w):
                q.append(w)

    all_cubes = itertools.product(
        range(xmin + 1, xmax), range(ymin + 1, ymax), range(zmin + 1, zmax)
    )
    return set(c for c in all_cubes if c not in occupied)


def main():
    lines = sys.stdin.readlines()
    cubes = set(tuple(map(int, line.split(","))) for line in lines)

    area = sum_surface_area(cubes)
    print(area)

    unvisited = flood(cubes)
    print(area - sum_surface_area(unvisited))


if __name__ == "__main__":
    main()
