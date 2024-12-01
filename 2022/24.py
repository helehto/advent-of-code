#!/usr/bin/env python3
import heapq
import math
import sys


def step_hurricanes(hurricanes, m, n):
    def step(x, y, d):
        if d == "^":
            return x, (y - 2) % (n - 2) + 1, d
        elif d == "v":
            return x, y % (n - 2) + 1, d
        elif d == "<":
            return (x - 2) % (m - 2) + 1, y, d
        elif d == ">":
            return x % (m - 2) + 1, y, d

    return tuple(step(*h) for h in hurricanes)


def neighbors(v, hurricanes, goal, m, n):
    if v not in hurricanes:
        yield v

    for dx, dy in ((-1, 0), (1, 0), (0, 1), (0, -1)):
        w = (v[0] + dx, v[1] + dy)
        if w == goal:
            yield goal
        elif 0 < w[0] < m - 1 and 0 < w[1] < n - 1 and w not in hurricanes:
            yield w


def main():
    lines = sys.stdin.readlines()

    initial_hurricanes = tuple(
        (x, y, c)
        for y, line in enumerate(lines[1:-1], 1)
        for x, c in enumerate(line.strip()[1:-1], 1)
        if c != "."
    )

    n = len(lines)
    m = len(lines[0].strip())
    start = (1, 0)
    goal = (lines[-1].index("."), n - 1)

    hurricane_table = [initial_hurricanes]
    for _ in range(math.lcm(m - 2, n - 2) - 1):
        hurricane_table.append(step_hurricanes(hurricane_table[-1], m, n))
    hurricane_table = [set(h[:2] for h in hs) for hs in hurricane_table]

    def search(t, start, goal):
        def heuristic(v, goal):
            return abs(v[0] - goal[0]) + abs(v[1] - goal[1])

        g = {(start, t): 0}
        q = [(heuristic(start, goal), start, t)]
        qset = set(q)

        while q:
            vh = heapq.heappop(q)
            _, v, t = vh
            vt = vh[1:]
            qset.remove(vh)

            if v == goal:
                return g[vt], t

            hurricanes = hurricane_table[(t + 1) % len(hurricane_table)]
            for w in neighbors(v, hurricanes, goal, m, n):
                wh = (w, t + 1)
                tentative = g.get(vt, math.inf) + 1
                if tentative < g.get(wh, math.inf):
                    g[wh] = tentative
                    k = (tentative + heuristic(w, goal), *wh)
                    if k not in qset:
                        heapq.heappush(q, k)
                        qset.add(k)

    t = 0
    cost1, t = search(t, start, goal)
    cost2, t = search(t, goal, start)
    cost3, t = search(t, start, goal)
    print(cost1)
    print(cost1 + cost2 + cost3)


if __name__ == "__main__":
    main()
