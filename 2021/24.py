#!/usr/bin/env python3
import itertools

# Hard-coded constants in input:
k1 = [1, 1, 1, 26, 1, 1, 26, 1, 26, 1, 26, 26, 26, 26]
k2 = [15, 14, 11, -13, 14, 15, -7, 10, -12, 15, -16, -9, -8, -8]
k3 = [4, 16, 14, 3, 11, 13, 11, 7, 12, 15, 13, 1, 15, 4]

# This is a rough equivalent of the compiled function in Python, annotated:
def f(input):
    z = 0
    digs = []
    for i, w in enumerate(input):
        # Summary: Pop a base-26 digit from z and then append w+k3[i] as a
        # base-26 digit if z % 26 + k2[i] == w.
        #
        # We unconditionally push digits when k1[i] == 1, as x will always be
        # true, so not much can be done about that. However, we can affect
        # things when k1[i] == 26, so we must ensure that a number is *never*
        # pushed in that case; otherwise, there will be a remainder in z after
        # the last iteration of the loop, so the model number will be invalid.
        #
        # In other words, this means that we want z % 26 + k2[i] to be equal to
        # w. z % 26 is the last base-26 digit that was pushed that has not been
        # popped yet. For instance, in iteration i == 3, this will be the value
        # input[2] + k3[2]. Hence, input[3] must be equal to input[2] + k3[2] +
        # k2[3].
        #
        # This logic applies to all input values for which k1[i] is 26, so we
        # have:
        #
        #    input[3] = input[2] + k3[2] + k2[3]
        #    input[6] = input[5] + k3[5] + k2[6]
        #    input[8] = input[7] + k3[7] + k2[8]
        #    input[10] = input[9] + k3[9] + k2[10]
        #    input[11] = input[4] + k3[4] + k2[11]
        #    input[12] = input[1] + k3[1] + k2[12]
        #    input[13] = input[0] + k3[0] + k2[13]
        #
        # ...which perfectly balances out to leave z = 0 when all input has
        # been consumed.

        x = int(z % 26 + k2[i] != w)
        z //= k1[i]

        if x:
            # Push w + k3[i] onto z as a base-26 digit.
            z = z * 26 + (w + k3[i])

    return z


def brute(digits):
    for d in itertools.product(digits, repeat=7):
        input = [d[0], d[1], d[2], 0, d[3], d[4], 0, d[5], 0, d[6], 0, 0, 0, 0]

        input[3] = input[2] + k3[2] + k2[3]
        input[6] = input[5] + k3[5] + k2[6]
        input[8] = input[7] + k3[7] + k2[8]
        input[10] = input[9] + k3[9] + k2[10]
        input[11] = input[4] + k3[4] + k2[11]
        input[12] = input[1] + k3[1] + k2[12]
        input[13] = input[0] + k3[0] + k2[13]

        if all(0 < x <= 9 for x in input):
            assert f(input) == 0
            return ''.join(map(str, input))

print(brute(range(9, 0, -1)))
print(brute(range(1, 10)))
