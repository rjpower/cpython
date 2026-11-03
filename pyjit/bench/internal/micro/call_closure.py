"""Closure call, one positional argument, reading one cell."""


def make():
    k = 1
    def inner(a):
        return a + k
    return inner


def run(loops):
    f = make()
    for _ in range(loops):
        for i in range(1000):
            z = f(i)
