"""Instance attribute store at one site seeing two shapes in turn: classes A
and B set the same four attributes in opposite orders.  Loops over a list,
so the empty_list_loop skeleton is subtracted."""


class A:
    def __init__(self):
        self.a = 1; self.b = 2; self.c = 3; self.d = 4
class B:
    def __init__(self):
        self.d = 4; self.c = 3; self.b = 2; self.a = 1


def run(loops):
    objs = [A(), B()] * 500
    for _ in range(loops):
        for o in objs:
            o.a = 1
