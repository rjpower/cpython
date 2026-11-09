"""Instance creation, empty __init__."""


class E:
    def __init__(self):
        pass


def run(loops):
    for _ in range(loops):
        for i in range(1000):
            z = E()
