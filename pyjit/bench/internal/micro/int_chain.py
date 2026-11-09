"""A chain of five int operations on small ints."""


def run(loops):
    x = 12345; y = 678
    for _ in range(loops):
        for i in range(1000):
            z = ((x + y) * 3 - x) // 7 + y
