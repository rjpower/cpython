"""str building by join: append a two-character str 1000 times, then join;
per iteration, the join amortised."""


def run(loops):
    for _ in range(loops):
        parts = []
        for i in range(1000):
            parts.append("ab")
        s = "".join(parts)
