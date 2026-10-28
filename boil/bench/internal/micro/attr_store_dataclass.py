"""Instance attribute store to an existing field of an 8-field dataclass,
monomorphic."""

import dataclasses


@dataclasses.dataclass
class O:
    a: int = 1
    b: int = 2
    c: int = 3
    d: int = 4
    e: int = 5
    f: int = 6
    g: int = 7
    h: int = 8


def run(loops):
    o = O()
    for _ in range(loops):
        for i in range(1000):
            o.h = i
