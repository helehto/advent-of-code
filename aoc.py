#!/usr/bin/env python
import argparse
import json
import math
import re
from tabulate import tabulate, SEPARATING_LINE
import subprocess


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--iterations", type=int, default=1)
    parser.add_argument("problems", nargs="+")
    args = parser.parse_args()

    cmd = ["./aoc", "-i", str(args.iterations), *args.problems]

    a = subprocess.check_output(cmd).strip()

    rows = []
    mean_sum = 0
    pct10_sum = 0
    pct90_sum = 0
    for year, day, ts in json.loads(a.split(b"\n")[-1]):
        scale = 1e-3
        ts = sorted(t * scale for t in ts)
        mean = sum(ts) / len(ts)

        def pct(k):
            return ts[int(len(ts) * k)]

        if args.iterations > 1:
            stddev = math.sqrt(sum((t - mean) ** 2 for t in ts) / (len(ts) - 1))
        else:
            stddev = "n/a"

        pct10 = pct(0.1)
        pct90 = pct(0.9)
        rows.append((year, day, mean, stddev, pct10, pct90))

        mean_sum += float(mean)
        pct10_sum += float(pct10)
        pct90_sum += pct90

    if len(rows) > 1:
        rows.append(SEPARATING_LINE)
        rows.append(['Σ', '', mean_sum, 0, pct10_sum, pct90_sum])

    headers = ("Year", "Day", "Mean (μs)", "σ (μs)", "10% (μs)", "90% (μs)")
    print(tabulate(rows, headers))


if __name__ == "__main__":
    main()
