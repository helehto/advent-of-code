#!/usr/bin/env python3
import re
import sys
import numpy as np

lines = []
for l in sys.stdin.readlines():
    mo = re.match(r"(\d+),(\d+) -> (\d+),(\d+)", l)
    lines.append(tuple(map(int, mo.groups())))

w = max(lines, key=lambda x: x[2])[2] + 2
h = max(lines, key=lambda x: x[3])[3] + 2

count1 = np.zeros((w, h))
count2 = np.zeros((w, h))

for x1, y1, x2, y2 in lines:
    if x1 == x2 or y1 == y2:
        xmin = min(x1, x2)
        xmax = max(x1, x2)
        ymin = min(y1, y2)
        ymax = max(y1, y2)
        count1[xmin:xmax+1, ymin:ymax+1] += 1
        count2[xmin:xmax+1, ymin:ymax+1] += 1
    else:
        assert x1 - x2 == y1 - y2 or x2 - x1 == y1 - y2

        if x1 > x2:
            x1, x2 = x2, x1
            y1, y2 = y2, y1

        if y1 < y2:
            for i in range(y2 - y1 + 1):
                count2[x1 + i, y1 + i] += 1
        else:
            for i in range(y1 - y2 + 1):
                count2[x1 + i, y1 - i] += 1

print(len(count1[count1 >= 2]))
print(len(count2[count2 >= 2]))
