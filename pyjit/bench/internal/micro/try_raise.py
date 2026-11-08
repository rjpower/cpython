"""try/except catching a raised ValueError."""


def run(loops):
    for _ in range(loops):
        for i in range(1000):
            try:
                raise ValueError
            except ValueError:
                pass
