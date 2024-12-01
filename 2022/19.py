#!/usr/bin/env python3
import functools
import itertools
from pathlib import Path
import numpy as np
import re
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from typing import *


def replace(t, i, v):
    return t[:i] + (v,) + t[i + 1 :]


@dataclass(slots=True)
class Constants:
    blueprint: Tuple
    target: int
    max_cost_per_material: Tuple = field(default=None)

    def __post_init__(self):
        self.max_cost_per_material = tuple(map(max, zip(*self.blueprint)))


def solve(c, cache, t, income, resources=(0,0,0), geodes=0):
    key = (income, resources)
    match cache.get(key):
        case (prev_time, prev_score):
            if prev_time < t or (prev_time == t and geodes < prev_score):
                return prev_score

    score = geodes

    # Recursive call: wait until we can afford to build a robot of type `i`,
    # then build it.
    for i in (3, 2, 1, 0):
        # If we can't ever build it with the income that we have right now,
        # skip it.
        if any(income[j] == 0 and c.blueprint[i][j] != 0 for j in range(3)):
            continue

        # If we are already generating more material of this type than any
        # single robot costs to build, then it isn't useful to build another
        # one - we would be generating more than we can possibly use. The
        # exception is geodes, which we enforce no limit on.
        if i != 3 and income[i] >= c.max_cost_per_material[i]:
            continue

        # Calculate the time needed to afford the robot.
        dt = 0
        for inc, cost, have in zip(income, c.blueprint[i], resources):
            if cost != 0:
                dt = max(dt, (cost - have + inc - 1) // inc)

        # Building the robot takes a cycle.
        dt += 1

        # Fast-forward time, unless it would cause us to overshoot.
        if t + dt < c.target:
            new_resources = tuple(
                have + inc * dt - cost
                for inc, cost, have in zip(income, c.blueprint[i], resources)
            )

            if i != 3:
                new_income = replace(income, i, income[i] + 1)
                new_geodes = geodes
            else:
                new_income = income
                new_geodes = geodes + c.target - (t + dt)

            score = max(score, solve(c, cache, t + dt, new_income, new_resources, new_geodes))

    cache[key] = (t, score)
    return score


def main():
    lines = sys.stdin.readlines()

    blueprints = []

    for line in lines:
        d = tuple(map(int, re.findall(r"(\d+)", line)))
        blueprints.append((
            (d[1], 0, 0),  # ore robot
            (d[2], 0, 0),  # clay robot
            (d[3], d[4], 0),  # obsidian robot
            (d[5], 0, d[6]),  # geode robot
        ))

    initial_income = (1, 0, 0)

    s = 0
    for i, bp in enumerate(blueprints, 1):
        c = Constants(blueprint=bp, target=24)
        s += i * solve(c, dict(), 0, initial_income)
    print(s)

    s = 1
    for i, bp in enumerate(blueprints[:3], 1):
        c = Constants(blueprint=bp, target=32)
        s *= solve(c, dict(), 0, initial_income)
    print(s)


if __name__ == "__main__":
    main()
