#!/usr/bin/env python3
import numpy as np
import sys


def treelines(ar, i, j):
    # (up, down, left, right) in the order seen from the tree at (i, j)
    return (
        list(reversed(ar[:i, j])),
        list(ar[i + 1 :, j]),
        list(reversed(ar[i, :j])),
        list(ar[i, j + 1 :]),
    )


def scenic_score(ar, i, j):
    def viewing_distance(v):
        for k, x in enumerate(v, 1):
            if x >= ar[i, j]:
                return k
        return len(v)

    d = [viewing_distance(x) for x in treelines(ar, i, j)]
    return d[0] * d[1] * d[2] * d[3]


def main():
    lines = [s.strip() for s in sys.stdin.readlines()]
    trees = np.array([[int(x) for x in row] for row in lines])
    visible = np.zeros(trees.shape, dtype=bool)
    n = len(trees)

    for i in range(1, n - 1):
        for j in range(1, n - 1):
            t = trees[i, j]
            vis = [(t > line).all() for line in treelines(trees, i, j)]
            visible[i, j] |= sum(vis)  # coerced to bool

    print(n * n - (n - 2) * (n - 2) + sum(np.ndarray.flatten(visible)))
    print(max(scenic_score(trees, i, j) for i, j in np.ndindex(trees.shape)))


if __name__ == "__main__":
    main()
