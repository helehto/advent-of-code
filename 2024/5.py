#!/usr/bin/env python3
import networkx as nx
import sys


def main():
    parts = sys.stdin.read().split("\n\n")

    G = nx.DiGraph()
    for line in parts[0].split():
        G.add_edge(*map(int, line.split("|")))

    solutions = [0, 0]
    for line in parts[1].split():
        a = tuple(map(int, line.split(",")))
        b = tuple(nx.topological_sort(G.subgraph(a)))
        solutions[b != a] += b[len(b) // 2]
    print(*solutions, sep='\n')


if __name__ == "__main__":
    main()
