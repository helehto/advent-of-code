#!/usr/bin/env python3
import re
import sys
import z3


def main():
    sections = sys.stdin.read().strip().split("\n\n")

    for offset in (0, 10000000000000):
        tokens = 0
        for i, section in enumerate(sections):
            ax, ay, bx, by, prizex, prizey = map(int, re.findall(r"\d+", section))

            # Using an SMT solver for this is killing a fly with a bazooka, but
            # my brain does not work at 6 in the morning ¯\_(ツ)_/¯
            s = z3.Solver()
            a, b = map(z3.Int, "a b".split())
            s.add(a * ax + b * bx == prizex + offset)
            s.add(a * ay + b * by == prizey + offset)
            try:
                s.check()
                if offset != 0:
                    print(
                        f"{i:>5}: a={s.model()[a].as_long()} b={s.model()[b].as_long()}"
                    )
                tokens += 3 * s.model()[a].as_long() + s.model()[b].as_long()
            except z3.z3types.Z3Exception:
                pass

        # print(tokens)


if __name__ == "__main__":
    main()
