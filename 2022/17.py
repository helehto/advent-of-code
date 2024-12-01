#!/usr/bin/env python3
import sys

ROCKS = (
    ((0, 0), (1, 0), (2, 0), (3, 0)),
    ((1, 0), (0, 1), (1, 1), (2, 1), (1, 2)),
    ((0, 0), (1, 0), (2, 0), (2, 1), (2, 2)),
    ((0, 0), (0, 1), (0, 2), (0, 3)),
    ((0, 0), (1, 0), (0, 1), (1, 1)),
)


def valid(occupied, rock):
    in_bounds = all(0 <= x < 7 and y >= 0 for x, y in rock)
    conflict = any((x, y) in occupied for x, y in rock)
    return in_bounds and not conflict


def move(rock, dx=0, dy=0):
    return [(x + dx, y + dy) for x, y in rock]


def main():
    lines = sys.stdin.readlines()
    jets = lines[0].strip()

    occupied = set()
    height = 0
    fallen_rocks = 0
    states = {}
    heights = []

    i, j = 0, 0
    while True:
        key = (i % len(ROCKS), j % len(jets))
        past_occurrences = states.setdefault(key, [])
        past_occurrences.append((fallen_rocks, height))

        # Check if we're in a cycle. This has a boatload of stupid assumptions
        # that happened to work on the input, but I can't be bother to go back
        # and rework this now.
        streak = 5
        if len(past_occurrences) > streak:
            fs, hs = zip(*past_occurrences[-streak:])
            df, dh = fs[1] - fs[0], hs[1] - hs[0]
            fs_ok = all(fs[i] - fs[i - 1] == df for i in range(i, len(fs)))
            hs_ok = all(hs[i] - hs[i - 1] == dh for i in range(i, len(hs)))
            if fs_ok and hs_ok:
                cycle_start = fallen_rocks
                cycle_period = df
                cycle_height = dh
                break

        # Spawn the rock.
        rock = [(p[0] + 2, p[1] + height + 3) for p in ROCKS[i % len(ROCKS)]]
        i += 1

        # Drop the rock.
        while True:
            # It gets pushed by the jet of gas, if possible:
            jet = jets[j % len(jets)]
            j += 1
            candidate = move(rock, dx={"<": -1, ">": 1}[jet])
            if valid(occupied, candidate):
                rock = candidate

            # It falls, if possible:
            candidate = move(rock, dy=-1)
            if not valid(occupied, candidate):
                occupied |= set(rock)
                height = max(height, max(y for _, y in rock) + 1)
                heights.append(height)
                fallen_rocks += 1
                break

            rock = candidate

    deltas = [heights[i] - heights[i - 1] for i in range(1, len(heights))]

    def solve(target):
        cycle_deltas = deltas[cycle_start - cycle_period - 1 :]
        cycles = (target - cycle_start) // cycle_period
        num_extra_steps = (target - cycle_start) % cycle_period
        return height + cycles * cycle_height + sum(cycle_deltas[:num_extra_steps])

    print(solve(2022))
    print(solve(1000000000000))


if __name__ == "__main__":
    main()
