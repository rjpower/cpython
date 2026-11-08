"""Comparison and branch, taken for half the iterations."""


def run(loops):
    for _ in range(loops):
        for i in range(1000):
            if i < 500:
                z = 1
            else:
                z = 2
