#!/usr/bin/env python3
import sys
lines = sys.stdin.read().strip().split('\n')

x, y = 0, 0
for line in lines:
    if line.startswith("forward"):
        x += int(line[8:])
    elif line.startswith("down"):
        y += int(line[5:])
    elif line.startswith("up"):
        y -= int(line[3:])

print(x * y)

x, y, aim = 0, 0, 0
for line in lines:
    if line.startswith("forward"):
        x += int(line[8:])
        y += aim * int(line[8:])
    elif line.startswith("down"):
        aim += int(line[5:])
    elif line.startswith("up"):
        aim -= int(line[3:])

print(x * y)
