#!/usr/bin/python

import sys

POINTS = int(sys.argv[1])
ENERGY = int(sys.argv[2]) if (len(sys.argv) > 2) else 161

def eval(points, energy):
    def check(points, wins, losses):
        return (wins * 50 + losses * 5) == points
    burned = 0
    while (energy >= 0):
        wins = energy
        losses = 0
        while (not check(points, wins, losses)) and (wins >= 0):
            wins -= 1
            losses += 1
        if check(points, wins, losses):
            return (wins, losses, burned)
        energy -= 1
        burned += 1
    return None

r = eval(POINTS, ENERGY)
if r is None:
    print("No solution found")
else:
    print("Wins: {} / Losses: {} / Burned: {}".format(*r))
