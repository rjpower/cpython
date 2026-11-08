"""int add with a cached small result."""


def run(loops):
    x = 1; y = 2
    for _ in range(loops):
        for i in range(1000):
            z = x + y
