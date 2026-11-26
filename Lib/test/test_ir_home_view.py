"""The homes view of a register instruction (_PyIR_WordHomes in
pycore_ir_outcomes.h) against the readers that decode stack shapes.

_testinternalcapi.ir_home_oracle lists, for every register instruction of a
code object, every scratch home and normal edge where the two give different
answers.  Each difference must be one that the declarations explain."""

import unittest

import _testinternalcapi

# _PyIR_PHYSICAL_*
PRESERVE, SET_NULL, UNKNOWN = 0, 1, 2
# _PyIR_OBSERVED_*
O_UNKNOWN, O_MISSING, O_VALUE, O_NULL = range(4)


def family(handler):
    return handler.split(".")[0]


def explained(row):
    """Why a difference is the declarations' answer, or None"""
    pc, handler, reader, edge, home, old, new = row
    fam = family(handler)
    if reader == "physical":
        if old == UNKNOWN and new in (SET_NULL, PRESERVE):
            return "a spent operand is left NULL; a kept or unnamed home is preserved"
        if fam in ("Send", "CleanupThrow") and edge == 1 and new == UNKNOWN:
            return "the completed value is stored outside the retired parts"
        if fam == "IterClose" and (old, new) == (PRESERVE, SET_NULL):
            return "IterClose retires its cursor"
    elif reader == "observed":
        if fam == "ForIter" and edge == 0 and (old, new) == ((O_NULL, 0), None):
            return "ForIter names no home above its pair"
        if fam == "BranchTruth" and edge == 1 and (old, new) == ((O_MISSING, 1), (O_UNKNOWN, 1)):
            return "a keep mode's jump leaves its operand in place"
        if fam == "IterClose" and (old, new) == (None, (O_MISSING, 1)):
            return "IterClose retires its cursor"
    elif reader == "nullable":
        if (old, new) == (False, True):
            return "the family declares the home vacant (vacant or Maybe<Src>)"
    elif reader == "absence":
        if old == "clear" and new in ("set", "keep"):
            return "the homes view answers where the stack shape did not"
        if fam == "IterClose" and (old, new) == ("keep", "set"):
            return "IterClose retires its cursor"
        if fam == "Send" and (old, new) == ("keep", "clear"):
            return "the completed value is stored outside the retired parts"
    elif reader == "touched":
        if fam == "IterClose" and (old, new) == (False, True):
            return "IterClose reads its cursor"
    return None


def unexplained(code):
    rows = []
    stack = [code]
    while stack:
        co = stack.pop()
        rows += [r for r in _testinternalcapi.ir_home_oracle(co) if explained(r) is None]
        stack += [c for c in co.co_consts if hasattr(c, "co_code")]
    return rows


SOURCES = {
    "loop with a local item": """
def f(xs):
    total = 0
    for x in xs:
        total += x
    return total
""",
    "loop with a temporary item": """
def f(xs):
    return [x * 2 for x in xs]
""",
    "early exit from a loop": """
def f(xs):
    for x in xs:
        if x:
            break
    return x
""",
    "delegation": """
def f(g):
    result = yield from g
    return result
""",
    "await": """
async def f(a):
    return (await a) + 1
""",
    "async for": """
async def f(a):
    async for x in a:
        pass
""",
    "with and except": """
def f(cm, xs):
    with cm as v:
        try:
            return xs[v]
        except (KeyError, IndexError) as e:
            return e
""",
    "keep-mode branches": """
def f(a, b):
    return a and b or None
""",
}


class HomeViewTests(unittest.TestCase):
    def test_differences_are_declared(self):
        for name, src in SOURCES.items():
            with self.subTest(name):
                code = compile(src, name, "exec")
                self.assertEqual(unexplained(code), [])

    def test_this_module(self):
        with open(__file__, encoding="utf-8") as f:
            code = compile(f.read(), __file__, "exec")
        self.assertEqual(unexplained(code), [])

    def test_completion_in_the_receivers_home(self):
        # The completed value of a delegation goes to the receiver's home
        # on the taken edge; the stack-shaped reader called it NULL there
        code = compile(SOURCES["delegation"], "d", "exec").co_consts[0]
        rows = [r for r in _testinternalcapi.ir_home_oracle(code)
                if r[2] == "physical" and family(r[1]) == "Send" and r[3] == 1]
        self.assertTrue(rows)
        for row in rows:
            self.assertEqual((row[5], row[6]), (SET_NULL, UNKNOWN))


if __name__ == "__main__":
    unittest.main()
