#!/usr/bin/env python3
import numpy as np
import re
import sys


FACE_ORIENTATIONS = [
    # 0
    {
        "U": (5, "L"),
        "L": (3, "L"),
        "D": (2, "U"),
        "R": (1, "L"),
    },
    # 1
    {
        "U": (5, "D"),
        "L": (0, "R"),
        "D": (2, "R"),
        "R": (4, "R"),
    },
    # 2
    {
        "U": (0, "D"),
        "L": (3, "U"),
        "D": (4, "U"),
        "R": (1, "D"),
    },
    # 3
    {
        "U": (2, "L"),
        "L": (0, "L"),
        "D": (5, "U"),
        "R": (4, "L"),
    },
    # 4
    {
        "U": (2, "D"),
        "L": (3, "R"),
        "D": (5, "R"),
        "R": (1, "R"),
    },
    # 5
    {
        "U": (3, "D"),
        "L": (0, "U"),
        "D": (1, "U"),
        "R": (4, "D"),
    },
]


TURN = np.array(((0, -1), (1, 0)), dtype=np.int32)


def score(y, x, d):
    facings = ((0, 1), (1, 0), (0, -1), (-1, 0))
    return 1000 * (y + 1) + 4 * (x + 1) + facings.index(tuple(d))


def move1(ar, p, d):
    while True:
        p = (p + d) % np.array(ar.shape)
        if ar[tuple(p)] != " ":
            break

    return p


def part1(ar, actions):
    d = np.array((0, 1), dtype=np.int32)
    p = move1(ar, np.array((0, -1)), d)

    for action in actions:
        if action == "L":
            d = TURN @ d
        elif action == "R":
            d = TURN.T @ d
        else:
            for _ in range(int(action)):
                npos = move1(ar, p, d)
                if ar[tuple(npos)] == "#":
                    break
                p = npos

    return score(p[0], p[1], tuple(d))


def part2(ar, actions):
    face_width = 50
    faces = np.array([-1] * ar.shape[0] * ar.shape[1]).reshape(ar.shape)
    face_to_global = []
    face = 0

    for i, j in np.ndindex(ar.shape):
        if ar[i, j] == "." and faces[i, j] < 0:
            faces[i : i + face_width, j : j + face_width] = face
            face_to_global.append(np.array((i, j)))
            face += 1

    def wrap(localp, face, d):
        y, x = localp
        if y < 0:
            exit_dir = "U"
            k = x
        elif y >= face_width:
            exit_dir = "D"
            k = x
        elif x < 0:
            exit_dir = "L"
            k = y
        elif x >= face_width:
            exit_dir = "R"
            k = y
        else:
            return localp, face, d

        nface, entry_dir = FACE_ORIENTATIONS[face][exit_dir]
        if exit_dir + entry_dir in ("LL", "RR", "UU", "DD", "RU", "UR", "LD", "DL"):
            k = face_width - 1 - k

        if entry_dir == "U":
            return (0, k), nface, (1, 0)
        elif entry_dir == "L":
            return (k, 0), nface, (0, 1)
        elif entry_dir == "D":
            return (face_width - 1, k), nface, (-1, 0)
        elif entry_dir == "R":
            return (k, face_width - 1), nface, (0, -1)

    face = 0
    local = (0, 0)
    d = np.array((0, 1))

    for action in actions:
        if action == "L":
            d = TURN @ d
        elif action == "R":
            d = TURN.T @ d
        else:
            for _ in range(int(action)):
                nlocal, nface, nd = wrap(np.array(local) + np.array(d), face, d)
                if ar[tuple(face_to_global[nface] + nlocal)] == "#":
                    break
                local, face, d = nlocal, nface, nd

    y, x = face_to_global[face] + local
    return score(y, x, d)


def main():
    chart, actions = sys.stdin.read().split("\n\n")
    chart = chart.split("\n")
    actions = re.findall(r"\d+|[LR]", actions)
    m = max(len(row) for row in chart)
    n = len(chart)
    ar = np.array([list(row + " " * (m - len(row))) for row in chart], dtype=str)
    print(part1(ar, actions))
    print(part2(ar, actions))


if __name__ == "__main__":
    main()
