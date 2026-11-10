"""Check actual keyword-plan hits and fallback controls on a stats build.

Run with a fresh -X pystats_dir directory and an output JSON path. These are
coverage counters, not timings; benchmark bodies remain separate and unchanged.
"""

import hashlib
import json
from pathlib import Path
import sys

import _testinternalcapi


def fixed(a, b, c):
    return a + b + c


def defaults(a, b=20, c=30):
    return a + b + c


def kwonly(a, *, b):
    return a + b


def optional(a, *, b=20):
    return a + b


def variadic(a, **kwargs):
    return a + kwargs["b"]


def four(a, b, c, d):
    return a + b + c + d


class Receiver:
    def method(self, a, b=20):
        return a + b


receiver = Receiver()
bound = receiver.method


def permutation(i):
    return fixed(c=i, a=i, b=i)


def prefix(i):
    return fixed(i, c=i, b=i)


def default_hole(i):
    return defaults(c=i, a=i)


def explicit_kwonly(i):
    return kwonly(b=i, a=i)


def implicit_self(i):
    return receiver.method(b=i, a=i)


def bound_wrapper(i):
    return bound(b=i, a=i)


def omitted_kwonly(i):
    return optional(a=i)


def variadic_fallback(i):
    return variadic(a=i, b=i)


def capacity_fallback(i):
    return four(a=i, b=i, c=i, d=i)


def main():
    directory = Path(sys._xoptions["pystats_dir"]).resolve()
    directory.mkdir(parents=True, exist_ok=False)
    output = Path(sys.argv[1])
    assert not output.exists()
    assert not sys._jit.is_enabled()
    cases = [
        (permutation, True, 3, 0),
        (prefix, True, 3, 0),
        (default_hole, True, 2, 20),
        (explicit_kwonly, True, 2, 0),
        (implicit_self, True, 2, 0),
        (bound_wrapper, False, 2, 0),
        (omitted_kwonly, False, 1, 20),
        (variadic_fallback, False, 2, 0),
        (capacity_fallback, False, 4, 0),
    ]
    rows = []
    for function, admitted, scale, constant in cases:
        for i in range(200):
            assert function(i) == scale * i + constant
        before = set(directory.glob("*.txt"))
        sys._stats_clear()
        sys._stats_on()
        for i in range(1000):
            assert function(i) == scale * i + constant
        assert sys._stats_dump()
        sys._stats_off()
        sys._stats_clear()
        [stats_file] = set(directory.glob("*.txt")) - before
        counters = dict(line.rsplit(":", 1) for line in
                        stats_file.read_text().splitlines() if ":" in line)
        hits = int(counters["Keyword binding plan hits"])
        assert hits >= 1000 if admitted else hits == 0, (function.__name__, hits)
        rows.append(dict(case=function.__name__, admitted=admitted, hits=hits,
                         stats_file=str(stats_file),
                         ir=_testinternalcapi.get_ir(function.__code__, True)))
    result = dict(git=sys._git, executable=sys.executable,
                  executable_sha256=hashlib.sha256(Path(sys.executable).read_bytes()).hexdigest(),
                  cases=rows)
    with output.open("x") as stream:
        json.dump(result, stream, indent=2)
        stream.write("\n")
    print(json.dumps({row["case"]: row["hits"] for row in rows}))


if __name__ == "__main__":
    main()
