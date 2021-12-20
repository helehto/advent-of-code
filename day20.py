#!/usr/bin/env python
import sys
import numpy as np

input = sys.stdin.read().strip().split('\n\n')
pattern = input[0].strip()
image = input[1]


g = np.array([[b.split() for b in g] for g in image.split()])
g = g.reshape((g.shape[0], g.shape[1]))
g = g == '#'


def neighborhood(g, i, j, pad):
    indices = ((-1, -1), (-1, 0), (-1, 1),
               (0, -1), (0, 0), (0, 1),
               (+1, -1), (+1, 0), (+1, 1))

    b = 0
    for y, x in indices:
        if 0 <= i + y < g.shape[0] and 0 <= j + x < g.shape[1]:
            b = b << 1 | int(g[i + y, j + x])
        else:
            b = b << 1 | int(pad)

    return b


outside_lit = False
for i in range(50):
    print(i)
    pad = pattern[0] == '#' and outside_lit
    g = np.pad(g, ((1,1), (1,1)), mode='constant',
               constant_values=(pad, pad))

    new_image = np.zeros(g.shape, dtype=bool)
    for i in range(g.shape[0]):
        for j in range(g.shape[1]):
            b = neighborhood(g, i, j, pad)
            new_image[i, j] = pattern[b] == '#'
    g = new_image

    outside_lit = not outside_lit

print(g.sum())
