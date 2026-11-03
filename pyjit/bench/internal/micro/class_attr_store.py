"""Store of an int to a class attribute (note 12 section 10)."""


class K:
    v = 0


def run(loops):
    for _ in range(loops):
        for i in range(1000):
            K.v = i
