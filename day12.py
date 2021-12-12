#!/usr/bin/env python3
import sys
from collections import Counter

class Node:
    def __init__(self, name):
        self.name = name
        self.edges = []
        self.small = self.name.islower()


def visited_twice(path):
    c = Counter(u.name for u in path if u.small)
    return any(v >= 2 for v in c.values())


def traverse(root, max_small):
    stack = [(root, (root,))]
    count = 0

    while stack:
        u, path = stack.pop()

        if u.name == 'end':
            count += 1
            continue

        s = 1 if visited_twice(path) else max_small

        for v in reversed(u.edges):
            c = sum(w == v for w in path)
            if not (c >= s and v.small) and not v.name == 'start':
                stack.append((v, path + (v,)))

    return count


caves = {}

for line in sys.stdin.read().strip().split():
    a, b = line.strip().split('-')
    a = caves.setdefault(a, Node(a))
    b = caves.setdefault(b, Node(b))
    a.edges.append(b)
    b.edges.append(a)

print(traverse(caves['start'], 1))
print(traverse(caves['start'], 2))
