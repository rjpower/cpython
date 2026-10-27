"""Class attribute load through an instance."""


class K:
    k = 1


def run(loops):
    o = K()
    for _ in range(loops):
        for i in range(1000):
            z = o.k
