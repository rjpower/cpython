"""Attribute store through a Python __setattr__ that drops the value
(note 12 section 10)."""


class S:
    def __setattr__(self, name, value):
        pass


def run(loops):
    s = S()
    for _ in range(loops):
        for i in range(1000):
            s.y = i
