"""Call of an instance with a Python __call__ that returns its int argument:
tp_call through slot_tp_call (note 12 section 10)."""


class C:
    def __call__(self, value):
        return value


def run(loops):
    c = C()
    for _ in range(loops):
        for i in range(1000):
            z = c(i)
