#!/usr/bin/env python3
import sys
import numpy as np
import itertools
from collections import Counter, defaultdict


input = sys.stdin.read().strip().split("\n")

# Part 1:
pos = [int(x.split(": ")[1]) for x in input]
scores = [0, 0]
die = itertools.cycle(range(1, 101))
for round in itertools.count(0):
    p = round % 2
    pos[p] = (pos[p] + sum(itertools.islice(die, 3)) - 1) % 10 + 1
    scores[p] += pos[p]
    if scores[p] >= 1000:
        break

print(scores[1 - p] * 3 * (round + 1))

# Part 2:
rolls = Counter(map(sum, itertools.product((1, 2, 3), repeat=3)))
cache = {}


def f(p1, p2, s1, s2, p1turn=True):
    if s1 >= 21:
        return [1, 0]
    if s2 >= 21:
        return [0, 1]

    key = (p1, p2, s1, s2, p1turn)
    m = cache.get(key)
    if m is not None:
        return m

    w = [0, 0]
    for sum, freq in rolls.items():
        if p1turn:
            newp = (p1 + sum - 1) % 10 + 1
            winner = f(newp, p2, s1 + newp, s2, not p1turn)
        else:
            newp = (p2 + sum - 1) % 10 + 1
            winner = f(p1, newp, s1, s2 + newp, not p1turn)

        w[0] += freq * winner[0]
        w[1] += freq * winner[1]

    cache[key] = w
    return w


pos = [int(x.split(": ")[1]) for x in input]
print(max(f(pos[0], pos[1], 0, 0)))
