"""str building by +=: append a two-character str 1000 times."""


def run(loops):
    for _ in range(loops):
        s = ""
        for i in range(1000):
            s += "ab"
