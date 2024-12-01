#!/usr/bin/env python3
import sys
input = [int(x) for x in sys.stdin.read().strip().split(',')]


def cost(l, target):
    return sum(abs(x - target) for x in l)


def cost2(l, target):
    def cost_single(a, b):
        dist = abs(a - b)
        return dist * (dist + 1) // 2
    return sum(cost_single(x, target) for x in l)


def minimize(l, f):
    return min(f(l, t) for t in range(min(input), max(input)+1))


print(minimize(input, cost))
print(minimize(input, cost2))
