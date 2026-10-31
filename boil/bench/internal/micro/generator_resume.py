"""Generator resume: a for loop over a generator yielding 1000 ints from its
own range loop.  One empty_loop skeleton is subtracted, so the value is the
resume, the yield and the generator's own loop step."""


def gen():
    for i in range(1000):
        yield i


def run(loops):
    for _ in range(loops):
        for x in gen():
            pass
