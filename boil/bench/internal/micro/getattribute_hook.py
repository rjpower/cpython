"""Attribute load through getattr() on a class with a Python
__getattribute__ returning an int (note 12 section 10)."""


class G:
    def __getattribute__(self, name):
        return 3000


def run(loops):
    g = G()
    for _ in range(loops):
        for i in range(1000):
            z = getattr(g, "y")
