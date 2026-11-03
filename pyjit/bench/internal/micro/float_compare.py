"""float compare."""


def run(loops):
    a = 1.5; b = 2.5
    for _ in range(loops):
        for i in range(1000):
            z = a < b
