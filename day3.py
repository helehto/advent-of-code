#!/usr/bin/env python3
import sys
lines = sys.stdin.read().strip().split('\n')

gamma, epsilon = 0, 0
for col in range(len(lines[0])):
    count = [0,0]
    for row in lines:
        count[int(row[col])] += 1
    b = 0 if count[0] > count[1] else 1
    gamma = gamma << 1 | b
    epsilon = epsilon << 1 | (1 - b)

print(gamma * epsilon)


candidates = list(lines)
col = 0
while len(candidates) > 1:
    count = [0,0]
    for row in candidates:
        count[int(row[col])] += 1

    b = '1' if count[0] <= count[1] else '0'
    candidates = [x for x in candidates if x[col] == b]
    col += 1
oxygen = int(candidates[0], 2)


candidates = list(lines)
col = 0
while len(candidates) > 1:
    count = [0,0]
    for row in candidates:
        count[int(row[col])] += 1

    b = '0' if count[0] <= count[1] else '1'
    candidates = [x for x in candidates if x[col] == b]
    col += 1
co2 = int(candidates[0], 2)

print(oxygen * co2)
