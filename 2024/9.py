#!/usr/bin/env python3
import sys


def checksum(disk):
    return sum(i * blk for i, blk in enumerate(disk) if isinstance(blk, int))


def build_disk(line):
    disk = []
    files = {}
    for i, c in enumerate(line):
        if i % 2 == 0:
            files[i // 2] = (len(disk), int(c))
            disk.extend([i // 2] * int(c))
        else:
            disk.extend(["."] * int(c))

    return disk, files


def part1(line):
    disk, _ = build_disk(line)
    i, j = 0, len(disk)
    while i + 1 < j:
        while isinstance(disk[i], int):
            i += 1
        j -= 1
        disk[i], disk[j] = disk[j], "."
    return checksum(disk)


def part2(line):
    disk, files = build_disk(line)
    for file_id, (file_pos, file_len) in reversed(files.items()):
        i = 0
        skip = False

        while True:
            while i < file_pos and isinstance(disk[i], int):
                i += 1
            if i >= file_pos or i >= len(disk):
                skip = True
                break

            j = i + 1
            while not isinstance(disk[j],int):
                j += 1
            if j - i >= file_len:
                break

            i = j+1

        if skip:
            continue

        disk[i : i + file_len] = [file_id] * file_len
        disk[file_pos : file_pos + file_len] = ["."] * file_len

    return checksum(disk)


def main():
    line = sys.stdin.read().strip()
    print(part1(line))
    print(part2(line))


if __name__ == "__main__":
    main()
