#!/usr/bin/env python3
import numpy as np
import sys


def main():
    lines = sys.stdin.readlines()

    paths = []
    for line in lines:
        parts = line.strip().split(" -> ")
        paths.append([list(map(int, s.split(",")))[::-1] for s in parts])

    miny = min(min(x[0] for x in y) for y in paths)
    maxy = max(max(x[0] for x in y) for y in paths)
    minx = min(min(x[1] for x in y) for y in paths)
    maxx = max(max(x[1] for x in y) for y in paths)
    floor = -1

    pad = 500
    halfpad = pad // 2

    input_map = np.zeros((maxy - miny + pad, maxx - minx + pad), dtype=str)
    input_map[:] = "."

    for path in paths:
        for (y0, x0), (y1, x1) in zip(path, path[1:]):
            x0 = x0 - minx + halfpad
            x1 = x1 - minx + halfpad
            y0 = y0 - miny + halfpad
            y1 = y1 - miny + halfpad
            x0, x1 = min(x0, x1), max(x0, x1)
            y0, y1 = min(y0, y1), max(y0, y1)
            assert x0 == x1 or y0 == y1
            if x0 == x1:
                input_map[y0 : (y1 + 1), x0] = "#"
            else:
                input_map[y0, x0 : x1 + 1] = "#"
                floor = max(floor, y0)

    def solve(m, floor_y=None):
        spawn = (0 - miny + halfpad, 500 - minx + halfpad)
        while m[spawn] == ".":
            p = np.array(spawn)
            while True:
                in_bounds = 1 <= p[0] < m.shape[0] - 1 and 1 <= p[1] < m.shape[1] - 1
                if not in_bounds:
                    return sum(np.ndarray.flatten(m == "o"))

                if p[0] + 1 == floor_y:
                    m[p[0], p[1]] = "o"
                    break

                for d in ((1,0), (1,-1), (1,1)):
                    if m[p[0] + d[0], p[1] + d[1]] == ".":
                        p += d
                        break
                else:
                    m[p[0], p[1]] = "o"
                    break

        return sum(np.ndarray.flatten(m == "o"))

    print(solve(np.array(input_map)))
    print(solve(np.array(input_map), floor + 2))


if __name__ == "__main__":
    main()
