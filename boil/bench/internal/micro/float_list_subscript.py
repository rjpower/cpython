"""Subscript of a list of floats by an int."""


def run(loops):
    l = [1.5] * 8; j = 3
    for _ in range(loops):
        for i in range(1000):
            z = l[j]
