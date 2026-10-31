"""isinstance of an instance against its class."""


class O:
    def __init__(self):
        self.a = 1; self.b = 2; self.c = 3; self.d = 4


def run(loops):
    o = O()
    for _ in range(loops):
        for i in range(1000):
            z = isinstance(o, O)
