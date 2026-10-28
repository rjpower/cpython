"""Method call on an instance of a type whose instances keep a dict word that
is still NULL: an exception (note 12 section 6.2)."""


class E(Exception):
    def m(self):
        return 1


def run(loops):
    e = E()
    for _ in range(loops):
        for i in range(1000):
            z = e.m()
