#!/usr/bin/env python3
import sys
import numpy as np
import re

dots = []
folds = []
for line in sys.stdin.read().strip().split('\n'):
    if line == '':
        continue

    m = re.match("fold along ([xy])=(\d+)", line)
    if m is not None:
        folds.append((m.group(1), int(m.group(2))))
    else:
        dots.append(tuple(map(int, line.split(','))))

w = 2 * max(c for dir, c in folds if dir == 'x') + 1
h = 2 * max(c for dir, c in folds if dir == 'y') + 1
g = np.zeros((h, w), dtype=bool)

for x, y in dots:
    g[y, x] = 1

for i, (dir, c) in enumerate(folds):
    if dir == 'x':
        g = g[:, :c] | np.flip(g[:, c+1:], axis=1)
    else:
        g = g[:c, :] | np.flip(g[c+1:, :], axis=0)

    if i == 0:
        print(g.sum())

for row in g:
    print(''.join('#' if c else ' ' for c in row))
