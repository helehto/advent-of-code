#!/usr/bin/env python3
import sys


def main():
    s = 0
    for line in (s.strip() for s in sys.stdin.readlines()):
        for i, c in enumerate(reversed(line)):
            s += ("=-012".index(c) - 2) * 5**i

    result = []
    while s:
        r = s % 5
        result.append("012=-"[r])
        s = s // 5 + (r >= 3)

    print("".join(result[::-1]))


if __name__ == "__main__":
    main()
