"""Store of a float through a property setter, which stores it in an
attribute (note 12 section 10)."""


class P:
    def __init__(self):
        self._x = 0.5
    @property
    def x(self):
        return self._x
    @x.setter
    def x(self, value):
        self._x = value


def run(loops):
    p = P()
    f = 2.5
    for _ in range(loops):
        for i in range(1000):
            p.x = f
