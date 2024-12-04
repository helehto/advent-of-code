#!/usr/bin/env python3
import functools
import itertools
import networkx as nx
import numpy as np
import re
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from typing import *


def read_word(a, ati, atj, dx, dy, l):
    try:
        w = []
        for q in range(l):
            ii = ati + q * dy
            jj = atj + q * dx
            if ii < 0 or jj < 0:
                return None
            w.append(a[ii][jj])
        return "".join(w)
    except:
        return None


def main():
    lines = sys.stdin.readlines()
    a = []
    for line in lines:
        if line := line.strip():
            a.append(line)

    w = len(a[0])
    h = len(a)

    n = 0
    for i in range(h):
        for j in range(w):
            for dx in (-1, 0, 1):
                for dy in (-1, 0, 1):
                    n += read_word(a, i, j, dx, dy, 4) == "XMAS"
    print(n)

    n = 0
    for i in range(h + 1):
        for j in range(w + 1):
            w1 = read_word(a, i - 1, j - 1, 1, 1, 3)
            w2 = read_word(a, i + 1, j - 1, 1, -1, 3)
            n += w1 in ("SAM", "MAS") and w2 in ("SAM", "MAS")
    print(n)


if __name__ == "__main__":
    main()
