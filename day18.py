#!/usr/bin/env python3
import functools
import json
import sys
import re
from dataclasses import dataclass, field
import typing as T


@dataclass
class Node:
    children: T.List[T.Union['Node', int]]
    parent: T.Optional['Node'] = field(default=None, repr=False)

    def __post_init__(self):
        for c in self.children:
            if type(c) is Node:
                c.parent = self

    def add_to_neighbor(self, addend, mirror):
        l = int(mirror)
        r = 1 - l

        node = self
        while True:
            child = node
            node = node.parent
            if node is None:
                return None
            if node.children[r] is child:
                break

        if type(node.children[l]) is int:
            node.children[l] += addend
        else:
            node = node.children[l]
            while type(node.children[r]) is Node:
                node = node.children[r]
            node.children[r] += addend

    @staticmethod
    def from_list(lst):
        children = []

        for i, c in enumerate(lst):
            if type(c) is list:
                children.append(Node.from_list(c))
            else:
                children.append(c)

        return Node(children)


def add(a, b):
    w = Node([a, b])

    def do_split(node):
        for i, c in enumerate(node.children):
            if type(c) is int and c >= 10:
                node.children[i] = Node([c // 2, (c + 1) // 2], node)
                return True
            if type(c) is Node and do_split(c):
                return True

    def do_explode(node, depth=0):
        if depth >= 4:
            node.add_to_neighbor(node.children[0], False)
            node.add_to_neighbor(node.children[1], True)
            child_index = int(node is node.parent.children[1])
            node.parent.children[child_index] = 0
            return True

        for c in node.children:
            if type(c) is Node and do_explode(c, depth + 1):
                return True

        return False

    while do_explode(w) or do_split(w):
        pass

    return w


def magnitude(n):
    if type(n) is int:
        return n
    return 3 * magnitude(n.children[0]) + 2 * magnitude(n.children[1])


lines = [json.loads(line) for line in sys.stdin.readlines()]
lists = [Node.from_list(c) for c in lines]

print(magnitude(functools.reduce(add, lists)))
print(max(magnitude(add(Node.from_list(a), Node.from_list(b)))
          for a in lines for b in lines))
