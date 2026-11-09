"""Builtin call: len of a three-item list."""


def run(loops):
    l = [1, 2, 3]
    for _ in range(loops):
        for i in range(1000):
            z = len(l)
