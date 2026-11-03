"""Attribute store through a Python data descriptor whose __set__ drops the
value (note 12 section 10)."""


class D:
    def __get__(self, obj, owner):
        return 3000
    def __set__(self, obj, value):
        pass


class H:
    d = D()


def run(loops):
    h = H()
    for _ in range(loops):
        for i in range(1000):
            h.d = i
