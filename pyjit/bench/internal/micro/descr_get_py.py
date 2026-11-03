"""Attribute load of a Python non-data descriptor whose __get__ returns an
int (note 12 section 10)."""


class D:
    def __get__(self, obj, owner):
        return 3000


class H:
    d = D()


def run(loops):
    h = H()
    for _ in range(loops):
        for i in range(1000):
            z = h.d
