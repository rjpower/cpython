"""Check eventual owner retirement across ordinary frame completion paths.

Run against a pinned native candidate. This tests reachability after collection,
not a finalizer order or a promise of immediate CPython destruction.
"""

import gc
import json
import weakref


class Value:
    pass


def ordinary(make):
    def consume(a, b=None, *args, **kwargs):
        return None
    consume(make(), make())
    consume(*(make(), make()), keyword=make())


def loop_return(make):
    for value in [make(), make()]:
        with_context = (make(), value)
        return None


def exception_exit(make):
    def fail():
        value = make()
        raise RuntimeError(value)
    try:
        fail()
    except RuntimeError:
        caught = make()
    finally:
        final = make()


def comprehension_exit(make):
    value = make()
    result = [(value, make()) for value in [make(), make()]]


def generator_exit(make):
    def generate():
        local = make()
        yield make()
        return make()
    generator = generate()
    next(generator)
    try:
        next(generator)
    except StopIteration:
        pass
    generator = generate()
    next(generator)
    generator.close()


def escaped_frame(make):
    import sys
    frames = []
    def escape():
        local = make()
        frames.append(sys._getframe())
    escape()
    # An exposed frame intentionally retains its bindings until cleared.
    frames.pop().clear()


def main():
    rows = []
    for case in (ordinary, loop_return, exception_exit, comprehension_exit,
                 generator_exit, escaped_frame):
        references = []
        def make():
            value = Value()
            references.append(weakref.ref(value))
            return value
        for _ in range(30):
            case(make)
        gc.collect()
        gc.collect()
        alive = sum(ref() is not None for ref in references)
        rows.append({"case": case.__name__, "created": len(references),
                     "alive_after_collection": alive})
    print(json.dumps(rows, indent=2))
    assert all(row["created"] and not row["alive_after_collection"] for row in rows)


if __name__ == "__main__":
    main()
