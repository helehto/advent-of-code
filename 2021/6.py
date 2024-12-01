#!/usr/bin/env python3
import sys
from collections import Counter

times = sys.stdin.read().strip().split(',')
times = [int(x) for x in times]


def lanternfish(days):
    c = Counter(times)
    c2 = Counter()

    for i in range(days):
        for t, count in c.items():
            if t == 0:
                c2[8] += count
                c2[6] += count
            else:
                c2[t - 1] += count

        c = Counter(c2)
        c2.clear()

    return sum(c.values())


print(lanternfish(80))
print(lanternfish(256))
