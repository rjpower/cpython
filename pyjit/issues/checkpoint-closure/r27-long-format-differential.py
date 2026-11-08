"""Deterministic upstream/candidate comparison for advanced int formatting."""
import json
import sys


class CustomInt(int):
    def __str__(self):
        return "custom-str"

    def __float__(self):
        return 2.5

    def __format__(self, spec):
        return "custom-format:" + spec


def outcome(fn):
    try:
        return {"result": fn()}
    except Exception as exc:
        return {"error": type(exc).__name__, "message": str(exc)}


def main():
    results = []

    def record(label, fn):
        results.append({"case": label, **outcome(fn)})

    values = [
        ("zero", 0), ("one", 1), ("minus-one", -1),
        ("inline-max", 2**50 - 1), ("inline-min", -(2**50 - 1)),
        ("boundary-positive", 2**50), ("boundary-negative", -(2**50)),
        ("heap-positive", 2**80), ("heap-negative", -(2**80)),
        ("huge", 2**4000),
    ]
    specs = ["", "b", "o", "d", "x", "X", "n", "c", "+#020x",
             " >25d", ",d", "_b", ".2f", ".3e", ".4g", "%",
             ".2d", "z", "#c", "q"]
    for name, value in values:
        for spec in specs:
            record(f"format/{name}/{spec}", lambda v=value, s=spec: format(v, s))
            record(f"field/{name}/{spec}", lambda v=value, s=spec: ("{0:" + s + "}").format(v))

    sub = CustomInt(42)
    for spec in ("", "d", ".2f"):
        record("subclass/format/" + spec, lambda s=spec: format(sub, s))
        record("subclass/int-format/" + spec, lambda s=spec: int.__format__(sub, s))
        record("subclass/field/" + spec, lambda s=spec: ("{0:" + s + "}").format(sub))
    record("float-overflow", lambda: format(10**400, ".2f"))
    original = sys.get_int_max_str_digits()
    try:
        sys.set_int_max_str_digits(640)
        value = 10**700
        record("digit-limit/decimal", lambda: format(value, "d"))
        record("digit-limit/hex", lambda: format(value, "x"))
    finally:
        sys.set_int_max_str_digits(original)
    print(json.dumps(results, ensure_ascii=True, separators=(",", ":")))


if __name__ == "__main__":
    main()
