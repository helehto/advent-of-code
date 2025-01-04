#!/usr/bin/env python3
import functools
import itertools
from itertools import permutations, product, pairwise
import networkx as nx
import numpy as np
import re
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from typing import *
import z3


def main():
    lines = [int(line.strip()) for line in sys.stdin.readlines()]

    secrets = []
    for line in lines:
        s = [line]
        while len(s) <= 2001:
            n = s[-1]
            n = ((n << 6) ^ n) & 16777215
            n = ((n >> 5) ^ n) & 16777215
            n = ((n << 11) ^ n) & 16777215
            s.append(n)
        secrets.append(s)
    print(sum(s[2000] for s in secrets))

    prices = []
    for s in secrets:
        prices.append(list(b % 10 - a % 10 for a, b in pairwise(s)))

    sequences = defaultdict(list)
    for p, s in zip(prices, secrets):
        seen = set()
        for i in range(4, len(p)):
            if (seq := tuple(p[i - 4 : i])) not in seen:
                sequences[seq].append(s[i] % 10)
                seen.add(seq)

    print(max(sum(v) for v in sequences.values()))


if __name__ == "__main__":
    main()
