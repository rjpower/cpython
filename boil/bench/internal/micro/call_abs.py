"""Builtin call: abs of a small int."""


def run(loops):
    x = -5
    for _ in range(loops):
        for i in range(1000):
            z = abs(x)
