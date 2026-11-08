"""Instance attribute load at one site seeing five shapes in turn, one more
than a site's four feedback rows: five classes set the same four
attributes.  Loops over a list, so the empty_list_loop skeleton is
subtracted."""


class A:
    def __init__(self):
        self.a = 1; self.b = 2; self.c = 3; self.d = 4
class B:
    def __init__(self):
        self.a = 1; self.b = 2; self.c = 3; self.d = 4
class C:
    def __init__(self):
        self.a = 1; self.b = 2; self.c = 3; self.d = 4
class D:
    def __init__(self):
        self.a = 1; self.b = 2; self.c = 3; self.d = 4
class E:
    def __init__(self):
        self.a = 1; self.b = 2; self.c = 3; self.d = 4


def run(loops):
    objs = [A(), B(), C(), D(), E()] * 200
    for _ in range(loops):
        for o in objs:
            z = o.a
