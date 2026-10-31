"""int add whose result is not a cached small int."""


def run(loops):
    x = 2000; y = 3000
    for _ in range(loops):
        for i in range(1000):
            z = x + y
