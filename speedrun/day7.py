#!/usr/bin/env python3
import sys
import functools
from dataclasses import dataclass, field
from typing import List, Any


@dataclass
class Dir:
    name: str = field(default="")
    children: List[Any] = field(default_factory=dict)
    parent: "Dir" = field(default=None)

    @property
    def size(self):
        s = 0
        for child in self.children.values():
            s += child.size
        return s

    def walk(self):
        yield self
        for child in self.children.values():
            if isinstance(child, Dir):
                yield from child.walk()


@dataclass
class File:
    size: int
    name: str
    parent: Dir


def main():
    lines = [s.strip() for s in sys.stdin.readlines()]
    lines = [s for s in lines if s != ""]

    root = Dir()
    curdir = root

    for line in lines:
        arg = line[5:]

        if line.startswith("$ cd"):
            if arg == "/":
                curdir = root
            elif arg == "..":
                assert curdir != root
                curdir = curdir.parent
            else:
                d = Dir(arg, parent=curdir)
                curdir = curdir.children.setdefault(arg, d)
        elif line.startswith("$ ls"):
            pass
        else:
            size, name = line.split()
            if size != "dir":
                size = int(size)
                curdir.children[name] = File(size, name, curdir)

    # This is probably horribly slow since we walk the entire directory
    # tree _many_ times. Oh well, it runs in less than a second...
    sizes = tuple(e.size for e in root.walk() if isinstance(e, Dir))

    print(sum(s for s in sizes if s <= 100000))

    unused = 70000000 - root.size
    needed = 30000000 - unused
    print(min(s for s in sizes if s >= needed))


if __name__ == "__main__":
    main()
