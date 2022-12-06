#!/usr/bin/env python3
import functools
import itertools
import numpy as np
import re
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass, field


def main():
    lines = sys.stdin.readlines()
    print(lines)

    # part 1
    for i in range(4, len(lines[0])):
        if len(set(lines[0][i - 4 : i])) == 4:
            print(i)
            break

    # part 2
    for i in range(4, len(lines[0])):
        if len(set(lines[0][i - 14 : i])) == 14:
            print(i)
            break


if __name__ == "__main__":
    main()
