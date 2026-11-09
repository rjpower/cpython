"""Compile-only inventory of actual local-load source and destination homes.

Run with the candidate CPython binary. This prints evidence; it does not
assume that a Python source form survives folding into a particular IR word.
"""

import json
import _testinternalcapi


K = 3


def constant_multiply(x):
    return x * K


def call_between_load_and_use(x, call):
    return x + call()


def walrus_before_replacement(x):
    return x + (x := x + 1)


def duplicate_then_store(x, call):
    result = x + call()
    return result


def local_copy(x, call):
    y = x
    call()
    return y


def nested_numeric(x, y):
    z = x * y
    return z + x


cases = (constant_multiply, call_between_load_and_use,
         walrus_before_replacement, duplicate_then_store,
         local_copy, nested_numeric)
inventory = {}
for function in cases:
    code = function.__code__
    nl = len(_testinternalcapi.get_co_localskinds(code))
    words = _testinternalcapi.get_ir(code)
    loads = []
    for pc, word in enumerate(words):
        name, a, b, *_ = word
        if name not in ("LOAD_FAST", "LOAD_FAST_BORROW"):
            continue
        loads.append({
            "pc": pc,
            "handler": name,
            "A": a,
            "B": b,
            "destination": ("sink" if a == 4095 else
                            "local" if a < nl else "temporary"),
            "same_home": a == b,
        })
    inventory[function.__name__] = {
        "nlocalsplus": nl,
        "loads": loads,
        "handlers": [word[0] for word in words],
    }
print(json.dumps(inventory, indent=2))
