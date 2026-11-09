"""Store through a property setter, which stores an int attribute
(note 12 section 10)."""


class P:
    def __init__(self):
        self._x = 0
    @property
    def x(self):
        return self._x
    @x.setter
    def x(self, value):
        self._x = value


def run(loops):
    p = P()
    for _ in range(loops):
        for i in range(1000):
            p.x = i
