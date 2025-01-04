#!/usr/bin/env python3
import networkx as nx
import sys


def main():
    G = nx.Graph()
    G.add_edges_from(line.strip().split("-") for line in sys.stdin.readlines())
    cq = list(nx.enumerate_all_cliques(G))
    print(sum(1 for c in cq if len(c) == 3 and any(x.startswith("t") for x in c)))
    print(",".join(sorted(cq[-1])))


if __name__ == "__main__":
    main()
