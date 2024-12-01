#!/usr/bin/env python3
import sys


HAND_VALUES = {"Y": 2, "Z": 3, "X": 1}
WIN = {"A": "Y", "B": "Z", "C": "X"}
DRAW = {"A": "X", "B": "Y", "C": "Z"}
LOSE = {"A": "Z", "B": "X", "C": "Y"}


def main():
    lines = [s.strip().split() for s in sys.stdin.readlines()]

    # part 1
    score = 0
    for a, b in lines:
        if b == WIN[a]:
            score += 6 + HAND_VALUES[b]
        elif b == DRAW[a]:
            score += 3 + HAND_VALUES[b]
    print(score)

    # part 2
    score = 0
    for a, b in lines:
        if b == "X":
            score += HAND_VALUES[LOSE[a]]
        elif b == "Y":
            score += 3 + HAND_VALUES[DRAW[a]]
        elif b == "Z":
            score += 6 + HAND_VALUES[WIN[a]]
    print(score)


if __name__ == "__main__":
    main()
