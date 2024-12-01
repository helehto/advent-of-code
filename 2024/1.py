#!/usr/bin/env python3
from collections import Counter
import numpy as np
import sys


def main():
    t = []
    for line in sys.stdin.readlines():
        if f := line.split():
            t.append(list(map(int, f)))

    a, b = np.sort(t, axis=0).T
    print(np.sum(abs(a - b)))

    c = Counter(b)
    print(sum(i * c.get(i, 0) for i in a))


if __name__ == "__main__":
    main()
