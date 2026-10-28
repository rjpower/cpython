"""Load of complex.real, a T_DOUBLE member (note 12 section 10)."""


def run(loops):
    c = complex(1.5, 2.5)
    for _ in range(loops):
        for i in range(1000):
            z = c.real
