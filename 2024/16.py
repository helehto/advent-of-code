#!/usr/bin/env python3
import networkx as nx
import sys


def main():
    G = nx.DiGraph()
    lines = sys.stdin.read().splitlines()
    for y, row in enumerate(lines):
        for x, c in enumerate(row.strip()):
            if c != "#":
                if c == "S":
                    start = (y, x, "E")
                if c == "E":
                    end = (y, x)

                for d1, d2 in zip("NESW", "ESWN"):
                    G.add_edge((y, x, d1), (y, x, d2), weight=1000)
                    G.add_edge((y, x, d2), (y, x, d1), weight=1000)

                for d in "NESW":
                    dy, dx = {"N": (-1, 0), "E": (0, 1), "S": (1, 0), "W": (0, -1)}[d]
                    in_bounds = 0 <= y + dx < len(lines) and 0 <= x + dx < len(row)
                    if in_bounds and lines[y + dy][x + dx] != "#":
                        G.add_edge((y, x, d), (y + dy, x + dx, d), weight=1)

    min_weight = min(nx.dijkstra_path_length(G, start, (*end, d)) for d in "NESW")
    print(min_weight)

    visited = set()
    for d in "NESW":
        for path in nx.all_shortest_paths(G, start, (*end, d), weight="weight"):
            if nx.path_weight(G, path, "weight") == min_weight:
                visited.update((y, x) for y, x, _ in path)
    print(len(visited))


if __name__ == "__main__":
    main()
