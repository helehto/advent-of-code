#!/usr/bin/env python3
import sys
import numpy as np
import itertools


a = []
for line in sys.stdin.readlines():
    a.append(list(line.strip()))

state = np.array(a)

for step in itertools.count(1):
    rmoves = []
    for i, j in np.ndindex(state.shape):
        nj = (j + 1) % state.shape[1]
        if state[i, j] == '>' and state[i, nj] == '.':
            rmoves.append(((i, j), (i, nj)))
    for f, t in rmoves:
        state[f], state[t] = '.', '>'

    dmoves = []
    for i, j in np.ndindex(state.shape):
        ni = (i + 1) % state.shape[0]
        if state[i, j] == 'v' and state[ni, j] == '.':
            dmoves.append(((i, j), (ni, j)))
    for f, t in dmoves:
        state[f], state[t] = '.', 'v'

    if not rmoves and not dmoves:
        break

print(step)
