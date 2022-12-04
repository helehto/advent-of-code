#!/usr/bin/env python3
import sys, re


def is_subrange(a0, a1, b0, b1):
    return (a0 >= b0 and a1 <= b1) or (b0 >= a0 and b1 <= a1)


def overlaps(a0, a1, b0, b1):
    return a0 <= b1 and b0 <= a1


def main():
    ranges = []
    for line in sys.stdin.read().split():
        x = [int(x) for x in re.findall(r"(\d+)-(\d+),(\d+)-(\d+)", line)[0]]
        ranges.append(x)

    print(sum(1 for x in ranges if is_subrange(*x)))
    print(sum(1 for x in ranges if overlaps(*x)))


if __name__ == "__main__":
    main()
