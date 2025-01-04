#!/usr/bin/env python3
import numpy as np
import sys


def main():
    paras = sys.stdin.read().split("\n\n")
    grids = np.array([list(map(list, g.splitlines())) for g in paras]) == "."
    print(sum((a | b).all() for i, a in enumerate(grids) for b in grids[i + 1 :]))


if __name__ == "__main__":
    main()
