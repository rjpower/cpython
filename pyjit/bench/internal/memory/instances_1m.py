"""1M four-attribute instances alive: a list of P(i) for i in range(1_000_000).

Each instance sets a = i, b = 1.5, c = None and d = "x" in __init__: one int
per instance (boxed from 256 up where a build boxes ints), the other three
shared constants.  Each loop builds the list and drops it.
"""


class P:
    def __init__(self, i):
        self.a = i
        self.b = 1.5
        self.c = None
        self.d = "x"


def run(loops):
    for _ in range(loops):
        alive = [P(i) for i in range(1_000_000)]
        del alive
