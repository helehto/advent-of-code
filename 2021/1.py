#!/usr/bin/env python3
import sys
input = sys.stdin.read().strip()
lines = [int(x) for x in input.split()]

print(sum(a < b for a, b in zip(lines[:-1], lines[1:])))
print(sum((True for i in range(1, len(lines)-2) if sum(lines[i:i+3]) > sum(lines[i-1:i+2]))))
