#!/usr/bin/env python3
import itertools
import sys


def main():
    elves = set(
        j + i * 1j
        for i, line in enumerate(sys.stdin.readlines())
        for j, c in enumerate(line)
        if c == "#"
    )

    dirs = (
        (-1j, 1 - 1j, -1 - 1j),  # N -> N, NE, NW
        (1j, 1 + 1j, -1 + 1j),  # S -> S, SE, SW
        (-1, -1 - 1j, -1 + 1j),  # W -> W, NW, SW
        (1, 1 - 1j, 1 + 1j),  # E -> E, NE, SE
    )

    neighbours = (-1 - 1j, -1j, 1 - 1j, -1, 1, -1 + 1j, 1j, 1 + 1j)

    for rnd in itertools.count(1):
        proposals = {}
        for p in elves:
            if all(p + n not in elves for n in neighbours):
                proposals[p] = p
                continue

            for scan in dirs:
                if all(p + d not in elves for d in scan):
                    dest = p + scan[0]

                    match proposals.get(dest):
                        case None:
                            proposals[dest] = p
                        case "conflict":
                            proposals[p] = p
                        case _:
                            proposals[proposals[dest]] = proposals[dest]
                            proposals[dest] = "conflict"
                            proposals[p] = p

                    break
            else:
                proposals[p] = p

        new_elves = set(k for k, v in proposals.items() if isinstance(v, complex))
        if elves == new_elves:
            print(rnd)
            break

        elves = new_elves

        if rnd == 10:
            xmin = int(min(p.real for p in elves))
            xmax = int(max(p.real for p in elves))
            ymin = int(min(p.imag for p in elves))
            ymax = int(max(p.imag for p in elves))
            print((xmax - xmin + 1) * (ymax - ymin + 1) - len(elves))

        dirs = dirs[1:] + (dirs[0],)


if __name__ == "__main__":
    main()
