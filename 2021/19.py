#!/usr/bin/env python
#
# This takes around 15 minutes to run on my computer. I regret nothing.

import functools
import json
import sys
import re
from dataclasses import dataclass, field
import typing as T
import numpy as np
import itertools

input = sys.stdin.read().strip().split('\n')

d = {}
scanner_count = None

for line in input:
    if line == '\n':
        continue

    m = re.match("--- scanner (\d+)", line)
    if m:
        scanner_count = int(m.group(1))

    m = re.match("(-?\d+),(-?\d+)(?:,(-?\d+))?", line)
    if m:
        x = int(m.group(1))
        y = int(m.group(2))
        z = int(m.group(3) or 0)
        d.setdefault(scanner_count, []).append(np.array(list(map(int, m.groups(0)))))

scanner_count += 1


def candidate_transforms(a, b):
    z = np.vstack((a, b)).T

    o1 = np.array(z[:, 0]).reshape((-1, 1))
    o2 = np.array(z[:, 1]).reshape((-1, 1))

    for xi, xsign in ((0, +1), (1, +1), (2, +1), (0, -1), (1, -1), (2, -1)):
        for yi, ysign in ((0, +1), (1, +1), (2, +1), (0, -1), (1, -1), (2, -1)):
            if xi == yi:
                continue

            R = np.zeros((3, 3))
            R[xi, 0] = xsign
            R[yi, 1] = ysign
            R[:, 2] = np.cross(R[:, 0], R[:, 1])
            assert np.linalg.det(R) == 1

            M = np.eye(4)
            M[:3, :3] = R
            M[:3, 3] = z[:,1] - R @ z[:,0]

            yield M


Ms = {(i, i): np.eye(4) for i in range(scanner_count)}

for s2 in range(1, scanner_count):
    print('@', s2)
    targets = set(map(tuple, d[s2]))

    def evaluate_points(ref1, ref2):
        # Find the most likely transform candidate.
        for M in candidate_transforms(ref1, ref2):
            hits = 0
            for testp in d[s1]:
                testp = np.array((*testp, 1))
                target = (M @ testp).astype(np.int32)
                target = tuple(target[:3])
                if target in targets:
                    hits += 1
                    if hits >= 12:
                        return M

    def find_transformation():
        for ref1 in d[s1]:
            for ref2 in d[s2]:
                M = evaluate_points(ref1, ref2)
                if M is not None:
                    return M

    for s1 in range(s2):
        ref1 = d[s1][0]
        ref2 = d[s2][0]

        for p in range(scanner_count):
            if p not in (s1, s2) and (s1, p) in Ms and (p, s2) in Ms:
                Ms[(s1, s2)] = Ms[(p, s2)] @ Ms[(s1, p)]
                Ms[(s2, s1)] = Ms[(p, s1)] @ Ms[(s2, p)]
                break
        else:
            M = find_transformation()
            if M is not None:
                Ms[(s1, s2)] = M
                Ms[(s2, s1)] = np.linalg.inv(M)

# Complete the set of transformations.
while True:
    change = False

    for i in range(scanner_count):
        for j in range(scanner_count):
            if (i, j) not in Ms:
                for k in range(scanner_count):
                    if i != k and j != k and (i, k) in Ms and (k, j) in Ms:
                        Ms[(i, j)] = Ms[(k, j)] @ Ms[(i, k)]
                        Ms[(j, i)] = np.linalg.inv(Ms[(i, j)])
                        change = True
                        break

    if not change:
        break

beacons = set()
for i in range(scanner_count):
    for s in d[i]:
        sx = np.array((*s, 1))
        gx = Ms[(i, 0)] @ sx
        beacons.add(tuple(gx[:3]))


print(len(beacons))

def manhattan(a, b):
    return sum(abs(i - j) for i, j in zip(a, b))

m = 0
for i in range(scanner_count):
    for j in range(scanner_count):
        a = (Ms[(i, 0)] @ [0, 0, 0, 1]).astype(np.int32)
        b = (Ms[(j, 0)] @ [0, 0, 0, 1]).astype(np.int32)
        m = max(m, manhattan(a[:3], b[:3]))
print(m)
