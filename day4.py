#!/usr/bin/env python3
import sys


def win(board, m):
    # rows
    for i in range(5):
        if all(m[i][j] for j in range(5)):
            return True

    # columns
    for i in range(5):
        if all(m[j][i] for j in range(5)):
            return True

    return False


def number_position(board, n):
    try:
        return next((i, j) for i in range(5) for j in range(5) if board[i][j] == n)
    except StopIteration:
        return None


def sum_unmarked(board, m):
    return sum(board[i][j] for i in range(5) for j in range(5) if not m[i][j])


def mark(board, marked, called):
    p = number_position(board, called)
    if p is None:
        return None
    i, j = p

    marked[i][j] = True

    if win(board, marked):
        return sum_unmarked(board, marked) * called

    return None


# Read input
numbers = [int(x) for x in sys.stdin.readline().strip().split(',')]
boards = []
while True:
    line = sys.stdin.readline()
    if line == '':
        break

    board = []
    assert line == '\n'
    for i in range(5):
        board.append([int(x) for x in sys.stdin.readline().strip().split()])

    boards.append(board)

marked = []
for i in range(len(boards)):
    marked.append([[False] * 5 for _ in range(5)])

done = [False] * len(boards)
left = len(boards)

for called in numbers:
    for bn, board in enumerate(boards):
        if done[bn]:
            continue

        s = mark(board, marked[bn], called)
        if s is not None:
            if left == len(boards):
                # Part 1
                print(s)
            left -= 1
            done[bn] = True
            if left == 0:
                # Part 2
                print(s)
                break

    if left == 0:
        break
