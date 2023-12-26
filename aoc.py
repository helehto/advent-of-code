#!/usr/bin/env python
import argparse
import json
import math
import re
from tabulate import tabulate, SEPARATING_LINE
import subprocess


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-T", "--total-only", action='store_true')
    parser.add_argument("other_args", nargs="+")
    args = parser.parse_args()

    cmd = ["./aoc", "--json", *args.other_args]

    a = subprocess.check_output(cmd).strip()

    rows = []
    means = []
    pct10_sum = 0
    pct90_sum = 0
    for year, day, ts in json.loads(a.split(b"\n")[-1]):
        scale = 1e-3
        ts = sorted(t * scale for t in ts)
        mean = sum(ts) / len(ts)

        def pct(k):
            return ts[int(len(ts) * k)]

        if len(ts) > 1:
            stddev = math.sqrt(sum((t - mean) ** 2 for t in ts) / (len(ts) - 1))
        else:
            stddev = 0

        pct10 = pct(0.1)
        pct90 = pct(0.9)
        if not args.total_only:
            rows.append((year, day, len(ts), mean, stddev, pct10, pct90))

        means.append(float(mean))
        pct10_sum += float(pct10)
        pct90_sum += pct90

    if len(rows) > 1 or args.total_only:
        if not args.total_only:
            rows.append(SEPARATING_LINE)
        rows.append(["Σ", "", sum(means), 0, pct10_sum, pct90_sum])

    headers = ("Year", "Day", "Iterations", "Mean (μs)", "σ (μs)", "10% (μs)", "90% (μs)")
    print(tabulate(rows, headers))

    if len(rows) > 1 or args.total_only:
        print()
        g = 2 ** (sum(math.log2(m) for m in means) / len(means))
        print(f"Geometric mean: {g:.2f} μs")


if __name__ == "__main__":
    main()
