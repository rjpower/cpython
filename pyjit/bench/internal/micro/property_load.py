"""Property load; the getter returns an attribute."""


class P:
    def __init__(self):
        self._x = 1
    @property
    def x(self):
        return self._x


def run(loops):
    p = P()
    for _ in range(loops):
        for i in range(1000):
            z = p.x
