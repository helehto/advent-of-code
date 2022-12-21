#!/usr/bin/env python3
import sys


def mix(indices, nums):
    for i in range(len(nums)):
        j = indices.index(i)
        indices.pop(j)
        indices.insert((j + nums[i]) % (len(nums) - 1), i)


def solve(nums, key, rounds):
    indices = list(range(len(nums)))
    nums = [n * key for n in nums]

    for _ in range(rounds):
        mix(indices, nums)

    nums = [nums[k] for k in indices]
    zero = nums.index(0)
    return sum(nums[(zero + 1000 * k) % len(nums)] for k in (1, 2, 3))


def main():
    nums = list(map(int, sys.stdin.read().split()))
    print(solve(nums, 1, 1))
    print(solve(nums, 811589153, 10))


if __name__ == "__main__":
    main()
