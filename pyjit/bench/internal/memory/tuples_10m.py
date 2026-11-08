"""10M two-tuples alive: a list of (i, i) for i in range(10_000_000).

The same Python objects on every build: one list, 10M tuples and, where a
build boxes ints, the ints from 256 up.  Each loop builds the list and drops
it, so the peak RSS is one list's worth.
"""


def run(loops):
    for _ in range(loops):
        alive = [(i, i) for i in range(10_000_000)]
        del alive
