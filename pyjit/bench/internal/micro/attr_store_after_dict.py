"""Instance attribute store after the instance's __dict__ was read."""


class O:
    def __init__(self):
        self.a = 1; self.b = 2; self.c = 3; self.d = 4


def run(loops):
    o = O(); o.__dict__
    for _ in range(loops):
        for i in range(1000):
            o.a = i
