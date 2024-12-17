#!/usr/bin/env python3
import itertools
import re
import sys


def step(robots, h, w):
    return [((px + vx) % w, (py + vy) % h, vx, vy) for px, py, vx, vy in robots]


def safety_factor(robots, h, w):
    q = [0, 0, 0, 0]
    for px, py, *_ in robots:
        if px < w // 2 and py < h // 2:
            q[0] += 1
        elif px < w // 2 and py > h // 2:
            q[1] += 1
        elif px > w // 2 and py > h // 2:
            q[2] += 1
        elif px > w // 2 and py < h // 2:
            q[3] += 1

    return q[0] * q[1] * q[2] * q[3]


def contains_tree(robots, h, w):
    visited = set()
    positions = set((px, py) for px, py, *_ in robots)
    unclustered = set(positions)

    clusters = []
    while unclustered:
        queue = [next(iter(unclustered))]
        cluster = set()
        while queue:
            u = queue.pop()
            if u in cluster:
                continue
            cluster.add(u)

            d = (-1, 0, 1)
            for dx, dy in itertools.product(d, d):
                v = (u[0] + dx, u[1] + dy)
                if v in positions:
                    queue.append(v)

        if len(cluster) > 30:
            return True
        unclustered -= cluster

    return False


def main():
    robots = []
    for line in sys.stdin.readlines():
        robots.append(list(map(int, re.findall("-?\d+", line))))

    w = 101 if len(robots) > 15 else 11
    h = 103 if len(robots) > 15 else 7

    for _ in range(100):
        robots = step(robots, h, w)
    print(safety_factor(robots, h, w))

    for i in range(100, 1_000_000):
        if contains_tree(robots, h, w):
            break
        robots = step(robots, h, w)
    print(i)


if __name__ == "__main__":
    main()
