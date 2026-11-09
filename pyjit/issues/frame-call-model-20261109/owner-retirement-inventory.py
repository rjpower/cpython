"""Bounded compile-only owner-retirement inventory; run with candidate Python."""
import json
from pathlib import Path
import sys
import types

from _testinternalcapi import get_ir


SOURCES = {
    "with": "def sample(cm):\n    with cm as value:\n        return value\n",
    "builtin_guards": "def sample(xs):\n    return any(x for x in xs)\n",
    "all_nested": "def sample(xs):\n    return all(all(y for y in x) for x in xs)\n",
    "break": "def sample(xs, ys):\n    for x in xs:\n        for y in ys:\n            if y:\n                break\n        if x:\n            break\n    return x\n",
    "finally_return": "def sample(x):\n    try:\n        return x()\n    finally:\n        return 2\n",
    "match": "def sample(x):\n    match x:\n        case [a, b] if a:\n            return b\n        case {'x': y}:\n            return y\n        case _:\n            return None\n",
    "except_star": "def sample(f):\n    try:\n        f()\n    except* ValueError as e:\n        print(e)\n",
    "comprehension": "def sample(xs):\n    return [f(x) for x in xs if x]\n",
}


def walk(code):
    yield code
    for value in code.co_consts:
        if isinstance(value, types.CodeType):
            yield from walk(value)


def describe(label, code, limit=24):
    words = get_ir(code)
    releases = [i for i, word in enumerate(words)
                if word[0] in {"POP_TOP", "POP_ITER"}]
    selected = releases[:limit]
    positions = list(code.co_positions())
    return {
        "source": label, "qualname": code.co_qualname,
        "firstline": code.co_firstlineno, "nlocals": code.co_nlocals,
        "ncellvars": len(code.co_cellvars), "nfreevars": len(code.co_freevars),
        "stacksize": code.co_stacksize, "words": len(words),
        "release_count": len(releases), "sample_limit": limit,
        "samples": [{"pc": i, "position": positions[i],
                     "neighbors": [{"pc": j, "handler": words[j][0],
                                    "A": words[j][1], "B": words[j][2],
                                    "C": words[j][3], "D": words[j][4]}
                                   for j in range(max(0, i - 1), min(len(words), i + 2))]}
                    for i in selected],
    }


def main():
    result = []
    for label, source in SOURCES.items():
        root = compile(source, f"<retirement:{label}>", "exec")
        for code in walk(root):
            if code.co_name != "<module>":
                result.append(describe(label, code))
    runtime = Path(sys.argv[1]) if len(sys.argv) > 1 else Path.cwd()
    path = runtime / "Lib/_pyrepl/utils.py"
    root = compile(path.read_text(), str(path), "exec")
    matches = [code for code in walk(root) if code.co_name == "is_soft_keyword_used"]
    assert len(matches) == 1
    result.append(describe("Lib/_pyrepl/utils.py", matches[0], limit=32))
    print(json.dumps({"python": sys.version, "compile_only": True,
                      "field_units": "physical word indices and raw A/B/C/D",
                      "codes": result}, indent=2))


if __name__ == "__main__":
    main()
