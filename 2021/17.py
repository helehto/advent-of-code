#!/usr/bin/env python3
import sys
import re


m = re.match(
    r"target area: x=(-?\d+)\.\.(-?\d+), y=(-?\d+)\.\.(-?\d+)",
    sys.stdin.read().strip())
x0, x1, y0, y1 = map(int, m.groups())


def step(x, y, vx, vy):
    x += vx
    y += vy
    if vx > 0:
        vx -= 1
    elif vx < 0:
        vx += 1
    vy -= 1
    return x, y, vx, vy


def run(vx, vy):
    x, y = 0, 0
    ymax = y

    while True:
        if vx == 0 and x < x0 or x > x1 or y < y0:
            return False, ymax
        if x0 <= x <= x1 and y0 <= y <= y1:
            return True, ymax

        x, y, vx, vy = step(x, y, vx, vy)
        ymax = max(ymax, y)


s = set()
ymax = 0
for vx in range(1, x1):
    for vy in range(y0, -y0):
        ok, h = run(vx, vy)
        if ok:
            s.add((vx, vy))
            ymax = max(ymax, h)

print(ymax)
print(len(s))
