#!/usr/bin/env python3
import sys


def is_safe(r):
    for a, b in zip(r, r[1:]):
        if not (1 <= abs(a - b) <= 3):
            return False

    for a, b, c in zip(r, r[1:], r[2:]):
        if not (a < b < c or a > b > c):
            return False

    return True


def is_safe_with_removal(r):
    for i in range(len(r)):
        r2 = r[:i] + r[i + 1 :]
        if is_safe(r2):
            return True

    return False


def main():
    reports = []
    for line in sys.stdin.readlines():
        if line := line.strip():
            reports.append(list(map(int, line.split())))

    print(sum(is_safe(r) for r in reports))
    print(sum(is_safe_with_removal(r) for r in reports))


if __name__ == "__main__":
    main()
