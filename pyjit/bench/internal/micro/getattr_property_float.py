"""Property load through getattr(), the executor's PROPERTY path; the getter
returns a float attribute (note 12 section 10)."""


class P:
    def __init__(self):
        self._x = 2.5
    @property
    def x(self):
        return self._x


def run(loops):
    p = P()
    for _ in range(loops):
        for i in range(1000):
            z = getattr(p, "x")
