"""object.__setattr__ called explicitly with an int, as a frozen dataclass's
__init__ does (note 12 section 10)."""


class O:
    def __init__(self):
        self.a = 0


def run(loops):
    o = O()
    setattr_ = object.__setattr__
    for _ in range(loops):
        for i in range(1000):
            setattr_(o, "a", i)
