"""try/except around a local store, no exception raised."""


def run(loops):
    for _ in range(loops):
        for i in range(1000):
            try:
                z = i
            except ValueError:
                pass
