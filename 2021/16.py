#!/usr/bin/env python3
import sys
import functools
import operator
from dataclasses import dataclass
import typing as T

@dataclass
class Packet:
    version: int
    type_id: int

    def evaluate(self):
        pass


@dataclass
class LiteralPacket(Packet):
    value: int

    def evaluate(self):
        return self.value

    def version_sum(self):
        return self.version


@dataclass
class OperatorPacket(Packet):
    operands: T.List[Packet]

    def evaluate(self):
        sub = [p.evaluate() for p in self.operands]
        if self.type_id == 0:
            return sum(sub)
        elif self.type_id == 1:
            return functools.reduce(operator.mul, sub)
        elif self.type_id == 2:
            return min(sub)
        elif self.type_id == 3:
            return max(sub)
        elif self.type_id == 5:
            return int(sub[0] > sub[1])
        elif self.type_id == 6:
            return int(sub[0] < sub[1])
        elif self.type_id == 7:
            return int(sub[0] == sub[1])

    def version_sum(self):
        return self.version + sum(p.version_sum() for p in self.operands)


def parse_packet(data):
    if not data:
        return None, None

    offset = 0

    def readbits(n):
        nonlocal offset
        b = data[offset:offset+n]
        offset += n
        return int(b, 2)

    version = readbits(3)
    type_id = readbits(3)

    if type_id == 4:
        value = 0
        done = False
        while not done:
            done = readbits(1) == 0
            value = value << 4 | readbits(4)

        return LiteralPacket(version, type_id, value), offset
    else:
        length_type_id = readbits(1)
        operands = []

        if length_type_id == 0:
            sublength = readbits(15)

            subdata = data[offset:offset+sublength]
            offset += sublength

            while True:
                operand, subl = parse_packet(subdata)
                if operand is None:
                    break
                subdata = subdata[subl:]
                operands.append(operand)
        else:
            for _ in range(readbits(11)):
                operand, subl = parse_packet(data[offset:])
                operands.append(operand)
                offset += subl

        return OperatorPacket(version, type_id, operands), offset


input = sys.stdin.read().strip()
data = bin(int(input, 16))[2:]

while len(data) % 4 != 0:
    data = '0' + data

packet, _ = parse_packet(data)
print(packet.version_sum())
print(packet.evaluate())
