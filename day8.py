#!/usr/bin/env python3
import sys
input = sys.stdin.read().strip().split('\n')

p1 = 0
p2 = 0
for line in input:
    segments, output = line.split('|')
    segments = [set(x) for x in segments.strip().split()]
    output = output.split()

    # Count easy digits for part 1.
    p1 += sum(True for digit in output if len(digit) in (2, 4, 3, 7))

    # I profusely apologize for producing the monstrosity below.
    d = {}
    d[1] = set(''.join(next(x for x in segments if len(x) == 2)))
    d[4] = set(''.join(next(x for x in segments if len(x) == 4)))
    d[7] = set(''.join(next(x for x in segments if len(x) == 3)))
    d[8] = set(''.join(next(x for x in segments if len(x) == 7)))
    d[3] = next(x for x in segments if len(x) == 5 and d[1] < set(x))
    d[2] = next(x for x in segments if len(x) == 5 and len(set(x) - d[4]) == 3)
    d[9] = next(x for x in segments if len(x) == 6 and len(set(x) - d[3]) == 1)
    d[0] = next(x for x in segments if len(x) == 6 and len(set(x) - d[9]) == 1 and d[1] < set(x))
    d[5] = next(x for x in segments if set(x) not in d.values() and len(x) == 5)
    d[6] = next(x for x in segments if set(x) not in d.values() and len(x) == 6)

    s = ''
    for digit in output:
        s += next(str(n) for n, seg in d.items() if set(digit) == seg)

    p2 += int(s)

print(p1)
print(p2)
