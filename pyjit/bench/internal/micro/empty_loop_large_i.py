"""The empty range loop over i >= 1025, past the small-int cache; whole loop, nothing subtracted."""


def run(loops):
    for _ in range(loops):
        for i in range(1025, 2025):
            pass
