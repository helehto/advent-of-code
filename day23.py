#!/usr/bin/env python3
import sys
import re
import itertools
from dataclasses import dataclass, field
import typing as T
import heapq


HALLWAY_STOPS = (0, 1, 3, 5, 7, 9, 10)
ROOMS = (2, 4, 6, 8)
ROOM_ADJ = ((1, 2), (2, 3), (3, 4), (4, 5))


def replaceitem(t, i, v):
    return t[:i] + (v,) + t[i+1:]


@dataclass
class State:
    hallway: T.Tuple[str]
    rooms: T.Tuple[T.Tuple[str, ...]]

    def astuple(self):
        return (self.hallway, self.rooms)

    def pop_room(self, room, dest):
        c = self.rooms[room][-1]
        new_hallway = replaceitem(self.hallway, dest, c)
        new_rooms = replaceitem(self.rooms, room, self.rooms[room][:-1])
        return State(new_hallway, new_rooms)

    def push_room(self, src, room):
        c = self.hallway[src]
        new_hallway = replaceitem(self.hallway, src, '.')
        new_rooms = replaceitem(self.rooms, room, self.rooms[room] + (c,))
        return State(new_hallway, new_rooms)


def target_room(amphipod):
    return ord(amphipod) - ord('A')


def weight(amphipod):
    return 10 ** (ord(amphipod) - ord('A'))


def hallway2room(state, room_capacity):
    new_states = []
    for stop_index, stop in enumerate(HALLWAY_STOPS):
        amphipod = state.hallway[stop]
        if amphipod == '.':
            continue

        room_index = target_room(amphipod)
        room_x = ROOMS[room_index]
        room = state.rooms[room_index]

        # Does the room contain misplaced amphipods?
        if any(room_index != target_room(c2) for c2 in room):
            continue

        # Check for collisions in the hallway.
        collision = False
        if room_x < stop:
            for t in itertools.count(stop_index - 1, -1):
                if HALLWAY_STOPS[t] < room_x:
                    break
                if state.hallway[HALLWAY_STOPS[t]] != '.':
                    collision = True
                    break
        else:
            for t in itertools.count(stop_index + 1, 1):
                if HALLWAY_STOPS[t] > room_x:
                    break
                if state.hallway[HALLWAY_STOPS[t]] != '.':
                    collision = True
                    break

        if not collision:
            slot = room_capacity - len(room)
            score = (abs(room_x - stop) + slot) * weight(amphipod)
            new_states.append((state.push_room(stop, room_index), score))

    return new_states


def room2hallway(state, room_capacity):
    new_states = []

    def add(stop_index):
        stop = HALLWAY_STOPS[stop_index]
        slot = room_capacity - len(room) + 1
        score = (abs(room_x - stop) + slot) * weight(room[-1])
        new_states.append((state.pop_room(room_index, stop), score))

    for room_index, (room_x, room) in enumerate(zip(ROOMS, state.rooms)):
        # If the room is empty, there is nothing to move.
        if not room:
            continue

        # Does this room contain misplaced amphipods? If not, no need to move
        # anything.
        if all(room_index == target_room(c) for c in room):
            continue

        # Generate moves for the top amphipod.
        stop_index = ROOM_ADJ[room_index][0]
        while stop_index >= 0 and state.hallway[HALLWAY_STOPS[stop_index]] == '.':
            add(stop_index)
            stop_index -= 1
        stop_index = ROOM_ADJ[room_index][1]
        while stop_index < len(HALLWAY_STOPS) and state.hallway[HALLWAY_STOPS[stop_index]] == '.':
            add(stop_index)
            stop_index += 1

    return new_states


def genmoves(state, room_capacity):
    return (room2hallway(state, room_capacity) +
            hallway2room(state, room_capacity))


def solve(init_state):
    @dataclass
    class PqState:
        score: int
        u: State

        def __lt__(self, o):
            return self.score < o.score

    room_capacity = max(len(room) for room in init_state.rooms)
    q = [PqState(0, init_state)]
    d = {init_state.astuple(): 0}

    while q:
        p = heapq.heappop(q)

        if p.score != d[p.u.astuple()]:
            continue

        for v, weight in genmoves(p.u, room_capacity):
            alt = p.score + weight
            if alt < d.get(v.astuple(), 1e9):
                d[v.astuple()] = alt
                heapq.heappush(q, PqState(alt, v))

    return tuple((s, score)
                 for s, score in d.items()
                 if score and s[0] == tuple('...........'))


def main():
    rows = []
    lines = sys.stdin.readlines()
    for line in lines[2:]:
        r = list(x for x in re.findall('[ABCD.]', line))
        if r:
            rows.append(r)

    hallway = tuple(lines[1][1:-2])
    rooms = tuple(tuple(reversed(x)) for x in zip(*rows))
    rooms = tuple(tuple(c for c in x if c != '.') for x in rooms)
    init_state = State(hallway, rooms)

    # Part 1
    print(solve(init_state)[0][1])

    init_state.rooms = replaceitem(init_state.rooms, 0, (init_state.rooms[0][0],) + ('D', 'D') + (init_state.rooms[0][1],))
    init_state.rooms = replaceitem(init_state.rooms, 1, (init_state.rooms[1][0],) + ('B', 'C') + (init_state.rooms[1][1],))
    init_state.rooms = replaceitem(init_state.rooms, 2, (init_state.rooms[2][0],) + ('A', 'B') + (init_state.rooms[2][1],))
    init_state.rooms = replaceitem(init_state.rooms, 3, (init_state.rooms[3][0],) + ('C', 'A') + (init_state.rooms[3][1],))
    print(solve(init_state)[0][1])

if __name__ == '__main__':
    main()
