"""Dict subscript by a str key."""


def run(loops):
    d = {'a': 1, 'b': 2, 'c': 3, 'd': 4}; k = 'c'
    for _ in range(loops):
        for i in range(1000):
            z = d[k]
