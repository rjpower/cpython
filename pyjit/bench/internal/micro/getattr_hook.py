"""Attribute load at a site that misses and calls a Python __getattr__
returning an int (note 12 section 10)."""


class G:
    def __getattr__(self, name):
        return 3000


def run(loops):
    g = G()
    for _ in range(loops):
        for i in range(1000):
            z = g.y
