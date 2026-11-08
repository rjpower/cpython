"""List subscript by an int."""


def run(loops):
    l = [1, 2, 3, 4, 5, 6, 7, 8]; j = 3
    for _ in range(loops):
        for i in range(1000):
            z = l[j]
