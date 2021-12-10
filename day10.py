#!/usr/bin/env python3
import sys
import functools
lines = sys.stdin.read().strip().split()

score1 = 0
scores2 = []

for line in lines:
    stack = []
    incorrect = False

    for c in line.strip():
        if c in '([{<':
            stack.append({'(': ')', '[': ']', '{': '}', '<': '>'}[c])
        else:
            if c != stack[-1]:
                score1 += {')': 3, ']': 57, '}': 1197, '>': 25137}[c]
                incorrect = True
            stack.pop()

    if stack and not incorrect:
        f = lambda score, x: 5 * score + ')]}>'.index(x) + 1
        scores2.append(functools.reduce(f, reversed(stack), 0))

print(score1)
scores2 = sorted(scores2)
print(scores2[len(scores2) // 2])
