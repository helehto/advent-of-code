#!/usr/bin/env python3
import sys


def main():
    lines = [s.strip() for s in sys.stdin.readlines()]

    # part 1
    prios = []
    for line in lines:
        if not line:
            break

        n = len(line) // 2
        a = set(line[:n])
        b = set(line[n:])

        c = next(iter(a & b))
        prio = 1 + "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ".index(c)
        prios.append(prio)
    print(sum(prios))

    # part 2
    prios = []
    for i in range(0, len(lines), 3):
        if not lines[i]:
            break

        a = set(lines[i]) & set(lines[i + 1]) & set(lines[i + 2])
        c = next(iter(a))
        prio = 1 + "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ".index(c)
        prios.append(prio)
    print(sum(prios))


if __name__ == "__main__":
    main()
