"""Load of cell.cell_contents, a getset over a cell holding an int that is
not a cached small int (6d rung 4)."""


def make_cell():
    x = 3000
    def f():
        return x
    return f.__closure__[0]


def run(loops):
    c = make_cell()
    for _ in range(loops):
        for i in range(1000):
            z = c.cell_contents
