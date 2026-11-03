"""Python function call, one positional argument; the function a module global."""


def f1(a):
    return a


def run(loops):
    for _ in range(loops):
        for i in range(1000):
            z = f1(i)
