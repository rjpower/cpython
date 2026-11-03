"""Builtin call: math.sin of a float, math a module global."""


import math


def run(loops):
    a = 0.5
    for _ in range(loops):
        for i in range(1000):
            z = math.sin(a)
