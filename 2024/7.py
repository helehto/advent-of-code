#!/usr/bin/env python3
import re
import sys


def find1(goal, a, nums):
    if not nums:
        return a == goal
    return find1(goal, a + nums[0], nums[1:]) or find1(goal, a * nums[0], nums[1:])


def find2(goal, a, nums):
    if not nums:
        return a == goal
    return (
        find2(goal, a + nums[0], nums[1:])
        or find2(goal, a * nums[0], nums[1:])
        or find2(goal, int(f"{a}{nums[0]}"), nums[1:])
    )


def main():
    lines = sys.stdin.readlines()

    for f in (find1, find2):
        n = 0
        for line in lines:
            goal, *nums = map(int, re.findall(r"\d+",line))
            if f(goal, 0, nums):
                n += goal
        print(n)


if __name__ == "__main__":
    main()
