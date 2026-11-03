"""__slots__ attribute load."""


class S:
    __slots__ = ("a", "b", "c", "d")
    def __init__(self):
        self.a = 1; self.b = 2; self.c = 3; self.d = 4


def run(loops):
    s = S()
    for _ in range(loops):
        for i in range(1000):
            z = s.a
