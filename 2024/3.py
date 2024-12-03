#!/usr/bin/env python3
import re
import sys


def main():
    text = sys.stdin.read()

    s = 0
    for a, b in re.findall(r"mul\((\d+)\s*,\s*(\d+)\)", text):
        s += int(a) * int(b)
    print(s)

    s = 0
    enable = True
    for m in re.finditer(r"do\(\)|don't\(\)|mul\((\d+)\s*,\s*(\d+)\)", text):
        if m.group(0) == "don't()":
            enable = False
        elif m.group(0) == "do()":
            enable = True
        elif enable:
            s += int(m.group(1)) * int(m.group(2))
    print(s)


if __name__ == "__main__":
    main()
