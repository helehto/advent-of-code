#!/usr/bin/env python3
import re
import sys
import z3


def main():
    lines = sys.stdin.readlines()
    sensors = [tuple(map(int, re.findall(r"(-?\d+)", line))) for line in lines]
    sensors = set((s[0], s[1], abs(s[0] - s[2]) + abs(s[1] - s[3])) for s in sensors)

    # I can't be bothered to switch between example/input manually
    target_y = 2000000 if len(sensors) > 20 else 10

    # This assumes that the entire range is contiguous. I was that lucky that
    # this worked out...
    xmin, xmax = int(1e12), int(-1e12)
    for x, y, d in sensors:
        target_dy = abs(target_y - y)
        if d >= target_dy:
            span = d - target_dy
            xmin = min(xmin, x - span)
            xmax = max(xmax, x + span)

    print(xmax - xmin)

    s = z3.Solver()
    x_ = z3.Int("x")
    y_ = z3.Int("y")

    for x, y, d in sensors:
        # abs() doesn't seem to be available in z3. We'd really like to say
        # abs(x - x_) + abs(y - y_) > d, but we have to split it into four
        # cases, blegh...
        s.add(
            z3.Or(
                z3.And(x - x_ > 0, y - y_ > 0, x - x_ + y - y_ > d),
                z3.And(x - x_ < 0, y - y_ > 0, x_ - x + y - y_ > d),
                z3.And(x - x_ < 0, y - y_ < 0, x_ - x + y_ - y > d),
                z3.And(x - x_ > 0, y - y_ < 0, x - x_ + y_ - y > d),
            )
        )

    s.add(x_ >= 0, x_ <= 4000000)
    s.add(y_ >= 0, y_ <= 4000000)
    s.check()
    print(s.model()[x_].as_long() * 4000000 + s.model()[y_].as_long())

if __name__ == "__main__":
    main()
