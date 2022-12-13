#!/usr/bin/env python3
import functools
import sys


def compare(a, b, indent=1):
    if isinstance(a, int) and isinstance(b, int):
        return a - b
    elif isinstance(a, list) and isinstance(b, list):
        for i, (x, y) in enumerate(zip(a, b)):
            if (k := compare(x, y)) != 0:
                return k
        return len(a) - len(b)
    else:
        a = a if isinstance(a, list) else [a]
        b = b if isinstance(b, list) else [b]
        return compare(a, b)


def main():
    packets = []
    for section in sys.stdin.read().strip().split("\n\n"):
        a, b = section.split()
        packets += (eval(a), eval(b))

    # part 1
    s = 0
    for i, (a, b) in enumerate(zip(packets[::2], packets[1::2]), 1):
        if compare(a, b) < 0:
            s += i
    print(s)

    # part 2
    packets += [[[2]], [[6]]]
    packets.sort(key=functools.cmp_to_key(compare))
    print((packets.index([[2]]) + 1) * (packets.index([[6]]) + 1))


if __name__ == "__main__":
    main()
