"""Load of a namedtuple field holding an int, through _tuplegetter
(note 12 section 10)."""

import collections

NT = collections.namedtuple("NT", "x y")


def run(loops):
    t = NT(3000, 2.5)
    for _ in range(loops):
        for i in range(1000):
            z = t.x
