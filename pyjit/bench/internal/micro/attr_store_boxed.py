"""Instance attribute store of a str to an existing attribute, monomorphic.
The field's first value is a str, so it is BOXED and the store's kind
guard always passes."""


class O:
    def __init__(self):
        self.a = ""; self.b = 2; self.c = 3; self.d = 4


def run(loops):
    o = O()
    v = "x"
    for _ in range(loops):
        for i in range(1000):
            o.a = v
