#!/usr/bin/env python3
import sys


def move_to_dir(m):
    return {
        "^": (-1, 0),
        "<": (0, -1),
        ">": (0, 1),
        "v": (1, 0),
    }[m]


def step1(robot, boxes, walls, move):
    dy, dx = move_to_dir(move)
    target = (robot[0] + dy, robot[1] + dx)

    p = target
    if p in walls:
        return robot
    if p not in boxes:
        return p

    while True:
        p = (p[0] + dy, p[1] + dx)
        if p in walls:
            return robot
        if p not in boxes:
            boxes.remove(target)
            boxes.add(p)
            return target


def step2(robot, boxes, walls, move):
    def box_at(p):
        if p in boxes:
            return p
        if (p[0], p[1] - 1) in boxes:
            return (p[0], p[1] - 1)
        return None

    dy, dx = move_to_dir(move)

    target = (robot[0] + dy, robot[1] + dx)
    if target in walls:
        return robot
    p = target

    firstbox = box_at(target)
    if firstbox is None:
        return target

    to_move = {firstbox}
    if dy != 0:
        queue = [firstbox]
        while queue:
            y, x = queue.pop()
            conflicts = set()
            for k in (-1, 0, 1):
                b = (y + dy, x + k)
                if b in boxes:
                    conflicts.add(b)
            queue.extend(conflicts)
            to_move |= conflicts
    else:
        step = 1 + (dx > 0)
        while p := box_at((p[0] + step * dy, p[1] + step * dx)):
            to_move.add(p)

    result_boxes = {(y + dy, x + dx) for y, x in to_move}
    if any((y, x + k) in walls for y, x in result_boxes for k in (0, 1)):
        return robot

    boxes -= to_move
    boxes |= result_boxes
    return target


def parse_grid(grid, k):
    boxes, walls = set(), set()
    for y, row in enumerate(grid.split()):
        for x, c in enumerate(row.strip()):
            if c == "@":
                robot = (y, k * x)
            elif c == "O":
                boxes.add((y, k * x))
            elif c == "#":
                walls |= {(y, k * x + dx) for dx in range(k)}

    return robot, boxes, walls


def solve(grid, moves, k):
    robot, boxes, walls = parse_grid(grid, k)
    step = step1 if k == 1 else step2

    for move in moves:
        robot = step(robot, boxes, walls, move)

    return sum(100 * y + x for y, x in boxes)


def main():
    sections = sys.stdin.read().strip().split("\n\n")
    grid, moves = sections
    moves = "".join(moves.splitlines())
    print(solve(grid, moves, 1))
    print(solve(grid, moves, 2))


if __name__ == "__main__":
    main()
