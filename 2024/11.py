#!/usr/bin/env python3
import sys
from collections import Counter, defaultdict


def step(counter):
    result = defaultdict(int)

    for num, count in counter.items():
        if num == 0:
            result[1] += count
        else:
            q = str(num)
            if len(q) % 2 == 0:
                result[int(q[: len(q) // 2])] += count
                result[int(q[len(q) // 2 :])] += count
            else:
                result[num * 2024] += count

    return result


def main():
    counter = Counter(map(int, sys.stdin.read().split()))

    for steps in (25, 50):
        for _ in range(steps):
            counter = step(counter)
        print(sum(counter.values()))


if __name__ == "__main__":
    main()
