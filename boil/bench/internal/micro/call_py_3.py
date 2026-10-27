"""Python function call, three positional arguments; the function a module global."""


def f3(a, b, c):
    return a


def run(loops):
    for _ in range(loops):
        for i in range(1000):
            z = f3(i, i, i)
