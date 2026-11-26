"""What a register instruction's edges leave in its homes.

The readers of Include/internal/pycore_ir_outcomes.h answer from the homes
each word names and the per-edge states its family's points declare
(_PyIRFormat.edges).  _testinternalcapi.ir_edge_homes gives, for one word of
a code object's lowering and one normal edge, the physical state of every
scratch home and, for a committed edge, the homes an observer sees.  The
cases below are the three answers that the readers which decoded stack
shapes got wrong.
"""

import unittest

from test.support import import_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")

# _PyIR_PHYSICAL_*
PRESERVE, SET_NULL, UNKNOWN = 0, 1, 2
# _PyIR_OBSERVED_*
O_UNKNOWN, O_MISSING, O_VALUE, O_NULL = range(4)
NEXT, TAKEN = 0, 1


def word_of(src, family):
    """The code object of f in src and the index and fields of its one word
    of family"""
    namespace = {}
    exec(compile(src, "<homes>", "exec"), namespace)
    code = namespace["f"].__code__
    found = [(i, word) for i, word in enumerate(_testinternalcapi.get_ir(code))
             if word[0].split(".")[0] == family]
    assert len(found) == 1, found
    return code, *found[0]


def edge_homes(code, index, edge):
    nl, physical, observed = _testinternalcapi.ir_edge_homes(code, index, edge)
    return nl, physical, None if observed is None else {
        home: (state, cut) for home, state, cut in observed}


class EdgeHomesTests(unittest.TestCase):

    def test_completed_value_in_the_receivers_home(self):
        # A delegation that completes stores its value in its destination,
        # here the receiver's home, after it retires the receiver, cursor and
        # sent value.  The old reader called that home NULL.
        code, index, (name, dst, base, *_) = word_of(
            "def f(g):\n    r = yield from g\n    return r\n", "Send")
        self.assertEqual(dst, base)
        nl, physical, observed = edge_homes(code, index, TAKEN)
        self.assertEqual(physical[dst - nl], UNKNOWN)
        self.assertEqual(physical[base + 1 - nl], SET_NULL)
        self.assertEqual(physical[base + 2 - nl], SET_NULL)
        self.assertEqual(observed[dst], (O_VALUE, 1))
        self.assertEqual(observed[base + 1], (O_MISSING, 1))

    def test_iter_close_retires_its_cursor(self):
        # The inlined any() leaves its loop through IterClose, which reads
        # and retires both parts of the pair.  The old reader did not name
        # the cursor and called its home preserved.
        code, index, (name, pair, *_) = word_of(
            "def f(a, xs):\n    return a and any(x for x in xs)\n", "IterClose")
        nl, physical, observed = edge_homes(code, index, NEXT)
        self.assertEqual(physical[pair - nl], SET_NULL)
        self.assertEqual(physical[pair + 1 - nl], SET_NULL)
        self.assertEqual(observed[pair + 1], (O_MISSING, 1))

    def test_keep_mode_jump_keeps_its_operand(self):
        # "a and b" jumps with a still in its home (point BranchTruth.Kept);
        # the fall-through retires it.  The old reader called the operand
        # missing on the taken edge.
        code, index, (name, operand, *_) = word_of(
            "def f(a):\n    return a.x and a.y\n", "BranchTruth")
        nl, physical, observed = edge_homes(code, index, TAKEN)
        self.assertGreaterEqual(operand, nl)
        self.assertNotEqual(observed[operand][0], O_MISSING)
        self.assertEqual(observed[operand][1], 1)

    def test_homes_the_word_does_not_name_are_preserved(self):
        code, index, (name, item, pair, *_) = word_of(
            "def f(xs):\n    return [x * 2 for x in xs]\n", "ForIter")
        nl, physical, _ = edge_homes(code, index, NEXT)
        named = {pair, pair + 1, item}
        for home in range(nl, nl + len(physical)):
            if home not in named:
                self.assertEqual(physical[home - nl], PRESERVE, home)
        self.assertEqual(physical[pair - nl], UNKNOWN)
        nl, physical, _ = edge_homes(code, index, TAKEN)
        self.assertEqual(physical[pair - nl], SET_NULL)
        self.assertEqual(physical[pair + 1 - nl], SET_NULL)


if __name__ == "__main__":
    unittest.main()
