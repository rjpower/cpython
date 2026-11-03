"""str building by += onto a long str: append a two-character str 1000
times to a local that starts as a 10,000-character str.  Each append
copies the whole string while += has no unshared-target proof (note 08's
register), so an iteration costs about 10,000 characters copied; with
linear appends it would cost what str_concat's does."""

BASE = "x" * 10_000


def run(loops):
    for _ in range(loops):
        s = BASE
        for i in range(1000):
            s += "ab"
