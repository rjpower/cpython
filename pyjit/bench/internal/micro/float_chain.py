"""A chain of five float operations."""


def run(loops):
    a = 1.5; b = 2.5
    for _ in range(loops):
        for i in range(1000):
            z = ((a + b) * 3.0 - a) / 7.0 + b
