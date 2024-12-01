#!/usr/bin/env python3
import sys


INSTR_TIMINGS = {
    "noop": 0,
    "addx": 1,
}


def main():
    X = 1
    cycle = 1
    pending_instr = None
    signal_strength = 0
    crt_rows = []

    # Main loop; 1 iteration per cycle
    lines = sys.stdin.readlines()
    while lines:
        # Start of the cycle. If nothing is executing, fetch an instruction.
        if pending_instr is None:
            line = lines.pop(0)
            op, *args = line.strip().split()
            pending_instr = (cycle + INSTR_TIMINGS[op], op, *map(int, args))

        # Get signal strength for part 1:
        if cycle in (20, 60, 100, 140, 180, 220):
            signal_strength += cycle * X

        # Draw to CRT for part 2:
        col = (cycle - 1) % 40
        if col == 0:
            # Push a new scanline
            crt_rows.append("")
        crt_rows[-1] += " #"[X - 1 <= col <= X + 1]

        # End of the cycle.
        if pending_instr[0] == cycle:
            # Current instruction is due; finish it.
            match pending_instr:
                case (_, "addx", arg):
                    X += arg
            pending_instr = None

        cycle += 1

    print(signal_strength)
    print("\n".join(crt_rows))


if __name__ == "__main__":
    main()
