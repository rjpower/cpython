"""Method call with no arguments returning a constant."""


class M:
    def m(self):
        return 1


def run(loops):
    o = M()
    for _ in range(loops):
        for i in range(1000):
            z = o.m()
