#!/usr/bin/env python3
import numpy as np
import sys


def move_knot(dest, src):
    src = list(src)
    dy = dest[0] - src[0]
    dx = dest[1] - src[1]

    if abs(dx) <= 1 and abs(dy) <= 1:
        # Valid state, don't move the knot
        return tuple(src)

    # I'm sure this can be simplified, I just can't be bothered to figure out
    # how by now
    if dest[0] == src[0]:
        src[1] += np.sign(dx)
    elif dest[1] == src[1]:
        src[0] += np.sign(dy)
    elif dest[0] < src[0] and dest[1] < src[1]:
        src[0] -= 1
        src[1] -= 1
    elif dest[0] < src[0] and dest[1] > src[1]:
        src[0] -= 1
        src[1] += 1
    elif dest[0] > src[0] and dest[1] > src[1]:
        src[0] += 1
        src[1] += 1
    elif dest[0] > src[0] and dest[1] < src[1]:
        src[0] += 1
        src[1] -= 1

    return tuple(src)


def solve(lines, n):
    # Head is index 0
    pos = [(30, 30) * 2 for _ in range(n)]
    tail_positions = {pos[-1]}

    def move(dy, dx):
        for _ in range(int(n)):
            # Move the head
            pos[0] = (pos[0][0] + dy, pos[0][1] + dx)

            # Update tail positions
            for i in range(1, len(pos)):
                pos[i] = move_knot(pos[i - 1], pos[i])
            tail_positions.add(pos[-1])

    for line in lines:
        d, n = line.split()

        if d == "U":
            move(-1, 0)
        elif d == "D":
            move(1, 0)
        elif d == "L":
            move(0, -1)
        elif d == "R":
            move(0, 1)

    return len(tail_positions)


def main():
    lines = sys.stdin.readlines()
    print(solve(lines, 2))
    print(solve(lines, 10))


if __name__ == "__main__":
    main()
