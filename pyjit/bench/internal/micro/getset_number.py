"""Load of memoryview.nbytes, a getset whose getter returns a number; 100,000
is not a cached small int (6d rung 4)."""


def run(loops):
    m = memoryview(bytes(100_000))
    for _ in range(loops):
        for i in range(1000):
            z = m.nbytes
