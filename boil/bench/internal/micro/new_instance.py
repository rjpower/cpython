"""Instance creation, four attributes set in __init__."""


class O:
    def __init__(self):
        self.a = 1; self.b = 2; self.c = 3; self.d = 4


def run(loops):
    for _ in range(loops):
        for i in range(1000):
            z = O()
