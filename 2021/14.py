#!/usr/bin/env python3
import sys
from collections import Counter

input = sys.stdin.read().strip().split('\n')
template = input[0]
rules = dict(line.split(' -> ') for line in input[2:])

bigrams = Counter(a + b for a, b in zip(template, template[1:]))
elements = Counter(template)

for i in range(1, 41):
    new_bigrams = Counter()

    for bigram, count in bigrams.items():
        t = rules[bigram]
        new_bigrams[bigram[0] + t] += count
        new_bigrams[t + bigram[1]] += count
        elements[t] += count

    bigrams = new_bigrams

    if i in (10, 40):
        print(max(elements.values()) - min(elements.values()))
