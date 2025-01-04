#!/usr/bin/env python3
from collections import defaultdict
import functools
from itertools import product
import sys


def shortest_single_level_moves(keys):
    result = defaultdict(list)
    keypos = {k: (i, j) for i, row in enumerate(keys) for j, k in enumerate(row)}
    blanky, blankx = keypos["!"]

    for k1, k2 in product(keypos, repeat=2):
        (ay, ax), (by, bx) = keypos[k1], keypos[k2]
        lr_moves = "<>"[bx > ax] * abs(bx - ax)
        ud_moves = "^v"[by > ay] * abs(by - ay)

        if ay == by:
            result[(k1, k2)].append("".join((lr_moves, "A")))
        if ax == bx:
            result[(k1, k2)].append("".join((ud_moves, "A")))
        if ay != blanky or bx != blankx:
            result[(k1, k2)].append("".join((lr_moves, ud_moves, "A")))
        if ax != blankx or by != blanky:
            result[(k1, k2)].append("".join((ud_moves, lr_moves, "A")))

    return result


MOVES_NUM = shortest_single_level_moves("789 456 123 !0A".split())
MOVES_DIR = shortest_single_level_moves("!^A <v>".split())


@functools.cache
def translate(code, depth):
    moves = translate_single(code, MOVES_NUM if code[0].isdigit() else MOVES_DIR)

    if depth:
        lens = [sum(translate(m, depth - 1) for m in move) for move in moves]
    else:
        lens = [sum(map(len, move)) for move in moves]

    return min(lens)


def translate_single(code, moves):
    code = "A" + code
    return product(*[moves[(a, b)] for a, b in zip(code, code[1:])])


def main():
    lines = [s.strip() for s in sys.stdin.readlines()]
    assert sum(translate(c, 2) * int(c[:-1]) for c in lines) == 163920
    assert sum(translate(c, 25) * int(c[:-1]) for c in lines) == 204040805018350


if __name__ == "__main__":
    main()
