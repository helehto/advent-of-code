#!/usr/bin/env python3
import sys
import functools
import operator
import numpy as np
import re


def overlaps(a, b):
    return not (a[0][1] < b[0][0] or a[0][0] > b[0][1] or
                a[1][1] < b[1][0] or a[1][0] > b[1][1] or
                a[2][1] < b[2][0] or a[2][0] > b[2][1])


def intersect(a, b):
    return ((max(a[0][0], b[0][0]), min(a[0][1], b[0][1])),
            (max(a[1][0], b[1][0]), min(a[1][1], b[1][1])),
            (max(a[2][0], b[2][0]), min(a[2][1], b[2][1])))


def volume(a):
    d = (a[i][1] - a[i][0] + 1 for i in range(3))
    return functools.reduce(operator.mul, d)


input = []

for line in sys.stdin.readlines():
    m = re.match(r"(off|on) x=(-?\d+)..(-?\d+),y=(-?\d+)..(-?\d+),z=(-?\d+)..(-?\d+)", line)
    input.append((
        m.group(1),
        ((int(m.group(2)), int(m.group(3))),
         (int(m.group(4)), int(m.group(5))),
         (int(m.group(6)), int(m.group(7))))))

# Part 1
ar = np.zeros((101, 101, 101), dtype=bool)
for state, ((x0, x1), (y0, y1), (z0, z1)) in input:
    x0 = max(x0, -50)
    y0 = max(y0, -50)
    z0 = max(z0, -50)
    x1 = min(x1, 50)
    y1 = min(y1, 50)
    z1 = min(z1, 50)
    ar[x0+50:x1+50+1, y0+50:y1+50+1, z0+50:z1+50+1] = state == 'on'

print(ar.sum())


# Part 2
regions = []
for i, (sa, a) in enumerate(input):
    new_regions = []

    for b, times in regions:
        if overlaps(a, b):
            ix = intersect(a, b)
            new_regions.append((ix, -times))

    if sa == 'on':
        new_regions.append((a, 1))

    regions += new_regions

print(sum(sign * volume(r) for r, sign in regions))
