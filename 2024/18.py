#!/usr/bin/env python3
from itertools import product
import networkx as nx
import re
import sys


def main():
    corrupted = [tuple(map(int, line.split(","))) for line in sys.stdin.readlines()]
    n = 71 if len(corrupted) > 30 else 7

    def G(k):
        c = set(corrupted[:k])
        g = nx.Graph()
        g.add_nodes_from(u for u in product(range(n), range(n)) if u not in c)
        for x, y in g:
            for d in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                v = (x + d[0], y + d[1])
                if 0 <= v[0] < n and 0 <= v[1] < n and v not in c:
                    g.add_edge((x, y), v)
        return g

    target = (n - 1, n - 1)
    print(nx.shortest_path_length(G(1024), (0, 0), target) - 1)

    bounds = [0, len(corrupted)]
    while bounds[0] + 1 < bounds[1]:
        k = sum(bounds) // 2
        bounds[not nx.has_path(G(k), (0, 0), target)] = k

    print(",".join(map(str, corrupted[bounds[0]])))


if __name__ == "__main__":
    main()
