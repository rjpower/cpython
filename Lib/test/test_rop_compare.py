"""The Compare, Identity, Contains and TestBranch families
(Python/register_ops/tests.rop): what each stack instruction lowers to, that
the answers are the language's, how a test and the jump on its answer become
one word, the far targets that word relaxes, the arms that adaptation picks,
and the branch events that monitoring sees.  Each arm against the C arm is
run by test_rop_ssa."""

import operator
import sys
import unittest

from test.support import import_helper

capi = import_helper.import_module("_testinternalcapi")

# The sys.monitoring events a branch reports
MON = sys.monitoring
TOOL = MON.OPTIMIZER_ID


def make(source):
    namespace = {}
    exec(source, namespace)
    return namespace["f"]


def words(f):
    "The names of the register instructions of f's code, without operands"
    return [w[0].split("+")[0] for w in capi.get_ir(f.__code__, True)
            if w[0] not in ("IR_OPERANDS", "OPERAND")]


def iter_tb(tb):
    while tb is not None:
        yield tb
        tb = tb.tb_next


def warm(f, *args, n=200):
    for _ in range(n):
        f(*args)


class Plain:
    pass


class Eq:
    "An instance whose equality is not its identity"

    def __eq__(self, other):
        return True

    __hash__ = None


class Raises:
    def __lt__(self, other):
        raise ZeroDivisionError("lt")

    def __contains__(self, item):
        raise ZeroDivisionError("contains")


class Bool:
    "A rich comparison whose result converts to bool by raising"

    class Result:
        def __bool__(self):
            raise ZeroDivisionError("bool")

    def __lt__(self, other):
        return Bool.Result()


OPERANDS = [0, 1, -1, 2**50, 2**60, 1.5, float("nan"), True, None, "a", "b", (1,), [1],
            {1}, {1: 2}, b"a", Plain(), Eq()]
RELATIONS = {"<": operator.lt, "<=": operator.le, "==": operator.eq,
             "!=": operator.ne, ">": operator.gt, ">=": operator.ge}


class LoweringTests(unittest.TestCase):
    def test_a_compare_whose_answer_is_returned(self):
        f = make("def f(a, b):\n    return a < b\n")
        self.assertEqual(words(f).count("Compare.Generic"), 1)
        self.assertFalse([n for n in words(f) if n.startswith("TestBranch")])

    def test_a_test_and_its_jump_are_one_word(self):
        for source, family in (("a < b", "TestBranch"), ("a is b", "TestBranch"),
                               ("a in b", "TestBranch"), ("a is not b", "TestBranch"),
                               ("a not in b", "TestBranch"), ("not a == b", "TestBranch")):
            f = make(f"def f(a, b):\n    if {source}:\n        return 1\n    return 2\n")
            with self.subTest(source=source):
                names = words(f)
                self.assertEqual(sum(n.startswith(family) for n in names), 1, names)
                for gone in ("Compare", "Identity", "Contains", "BranchTruth"):
                    self.assertFalse([n for n in names if n.startswith(gone)], names)

    def test_an_identity_test_has_no_feedback(self):
        f = make("def f(a, b):\n    if a is b:\n        return 1\n    return 2\n")
        self.assertIn("TestBranch.Identity", words(f))

    def test_a_while_loop_condition_is_a_test_branch(self):
        f = make("def f(n):\n    i = 0\n    while i < n:\n        i += 1\n    return i\n")
        self.assertTrue([n for n in words(f) if n.startswith("TestBranch")])
        self.assertEqual(f(5), 5)

    def test_no_fused_handlers_remain(self):
        from _opcode_metadata import _ir_opmap
        fused = [n for n in _ir_opmap
                 if n.startswith(("COMPARE_OP_", "CONTAINS_OP_", "IS_OP_"))
                 or n.startswith(("INSTRUMENTED_COMPARE_OP", "INSTRUMENTED_CONTAINS_OP",
                                  "INSTRUMENTED_IS_OP"))]
        self.assertEqual(fused, [])

    def test_the_compare_intrinsics_are_gone(self):
        import opcode
        names = opcode._intrinsic_2_descs
        self.assertFalse([n for n in names if "COMPARE" in n or "CONTAINS" in n], names)
        self.assertEqual(names[22:], ["INTRINSIC_GETATTR", "INTRINSIC_SETATTR"])


class AnswerTests(unittest.TestCase):
    def check(self, fn, expected, a, b):
        try:
            want = expected(a, b)
        except Exception as e:
            want = type(e)
        try:
            got = fn(a, b)
        except Exception as e:
            got = type(e)
        if want != want and got != got:   # nan
            return
        self.assertEqual(got, want, (a, b))

    def test_compare_in_every_form(self):
        for symbol, op in RELATIONS.items():
            rich = make(f"def f(a, b):\n    return a {symbol} b\n")
            truth = make(f"def f(a, b):\n    return not (a {symbol} b)\n")
            branch = make(f"def f(a, b):\n    if a {symbol} b:\n        return 1\n    return 2\n")
            for a in OPERANDS:
                for b in OPERANDS:
                    with self.subTest(symbol=symbol, a=a, b=b):
                        self.check(rich, op, a, b)
                        self.check(truth, lambda x, y: not op(x, y), a, b)
                        self.check(branch, lambda x, y: 1 if op(x, y) else 2, a, b)

    def test_identity_and_containment_in_every_form(self):
        cases = {
            "a is b": lambda a, b: a is b,
            "a is not b": lambda a, b: a is not b,
            "a in b": lambda a, b: a in b,
            "a not in b": lambda a, b: a not in b,
        }
        for source, op in cases.items():
            value = make(f"def f(a, b):\n    return {source}\n")
            branch = make(f"def f(a, b):\n    if {source}:\n        return 1\n    return 2\n")
            for a in OPERANDS:
                for b in OPERANDS:
                    with self.subTest(source=source, a=a, b=b):
                        self.check(value, op, a, b)
                        self.check(branch, lambda x, y: 1 if op(x, y) else 2, a, b)

    def test_a_failing_test_raises_from_its_line(self):
        for source in ("a < b", "a in b"):
            f = make(f"def f(a, b):\n    if {source}:\n        return 1\n    return 2\n")
            args = (Raises(), Raises()) if source == "a < b" else (1, Raises())
            for _ in range(3):
                with self.subTest(source=source):
                    try:
                        f(*args)
                    except ZeroDivisionError as e:
                        frames = [(t.tb_frame.f_code, t.tb_lineno)
                                  for t in iter_tb(e.__traceback__)]
                    else:
                        self.fail("no exception")
                    self.assertEqual(frames[1], (f.__code__, 2))

    def test_a_result_whose_truth_raises(self):
        f = make("def f(a, b):\n    if a < b:\n        return 1\n    return 2\n")
        for _ in range(3):
            with self.assertRaises(ZeroDivisionError):
                f(Bool(), 1)

    def test_a_test_in_a_handler_and_a_loop(self):
        f = make("def f(items, key):\n"
                 "    n = 0\n"
                 "    for x in items:\n"
                 "        try:\n"
                 "            if x < key:\n"
                 "                n += 1\n"
                 "        except TypeError:\n"
                 "            n += 100\n"
                 "    return n\n")
        for _ in range(5):
            self.assertEqual(f([1, 2, "a", 3], 3), 102)


class AdaptationTests(unittest.TestCase):
    def arms(self, source, *args, expect=None):
        f = make(source)
        warm(f, *args)
        return words(f)

    def test_compare_arms(self):
        template = "def f(a, b):\n    return a {} b\n"
        self.assertIn("Compare.Int", self.arms(template.format("<"), 1, 2))
        self.assertIn("Compare.Float", self.arms(template.format("<"), 1.5, 2.5))
        self.assertIn("Compare.Str", self.arms(template.format("=="), "a", "b"))
        self.assertIn("Compare.Generic", self.arms(template.format("<"), "a", "b"))

    def test_contains_arms(self):
        template = "def f(a, b):\n    return a in b\n"
        self.assertIn("Contains.Set", self.arms(template, 1, {1}))
        self.assertIn("Contains.Dict", self.arms(template, 1, {1: 2}))
        self.assertIn("Contains.Generic", self.arms(template, 1, [1]))

    def test_test_branch_arms(self):
        template = "def f(a, b):\n    if a {} b:\n        return 1\n    return 2\n"
        self.assertIn("TestBranch.Int", self.arms(template.format("<"), 1, 2))
        self.assertIn("TestBranch.Float", self.arms(template.format("<"), 1.5, 2.5))
        self.assertIn("TestBranch.Str", self.arms(template.format("=="), "a", "b"))
        self.assertIn("TestBranch.Set", self.arms(template.format("in"), 1, {1}))
        self.assertIn("TestBranch.Dict", self.arms(template.format("in"), 1, {1: 2}))
        self.assertIn("TestBranch.Sequence", self.arms(template.format("in"), 1, [1]))

    def test_a_miss_runs_the_generic_arm_and_adapts(self):
        f = make("def f(a, b):\n    if a < b:\n        return 1\n    return 2\n")
        warm(f, 1, 2)
        self.assertIn("TestBranch.Int", words(f))
        for _ in range(3):
            self.assertEqual(f("a", "b"), 1)
            self.assertEqual(f(1.5, 0.5), 2)


class FarTargetTests(unittest.TestCase):
    def body(self, n):
        return "".join(f"        x{i % 8} = {i}\n" for i in range(n))

    def test_a_far_forward_target_is_relaxed(self):
        source = ("def f(a, b):\n    r = 0\n    if a < b:\n" + self.body(2600)
                  + "        r = 1\n    return r\n")
        f = make(source)
        for _ in range(3):
            self.assertEqual(f(1, 2), 1)
            self.assertEqual(f(2, 1), 0)
        warm(f, 1, 2, n=100)
        self.assertEqual(f(1, 2), 1)
        self.assertEqual(f(2, 1), 0)

    def test_a_far_target_of_each_test(self):
        for test, args, other in (("a is b", (1, 1), (1, 2)), ("a in b", (1, [1]), (2, [1])),
                                  ("a == b", ("x", "x"), ("x", "y"))):
            source = (f"def f(a, b):\n    r = 0\n    if {test}:\n" + self.body(2600)
                      + "        r = 1\n    return r\n")
            f = make(source)
            with self.subTest(test=test):
                for _ in range(3):
                    self.assertEqual(f(*args), 1)
                    self.assertEqual(f(*other), 0)


class MonitorTests(unittest.TestCase):
    def setUp(self):
        MON.use_tool_id(TOOL, "test_rop_compare")
        self.events = []
        for name in ("BRANCH_LEFT", "BRANCH_RIGHT"):
            MON.register_callback(
                TOOL, getattr(MON.events, name),
                lambda code, src, dst, name=name: self.events.append((name, src, dst)))

    def tearDown(self):
        MON.free_tool_id(TOOL)

    def watch(self, f):
        MON.set_local_events(TOOL, f.__code__, MON.events.BRANCH_LEFT | MON.events.BRANCH_RIGHT)

    def jump_of(self, f):
        import dis
        return next(i for i in dis.get_instructions(f) if i.opname.startswith("POP_JUMP_IF"))

    def test_both_senses_of_every_test_report_their_edges(self):
        tests = ("a < b", "a is b", "a in b", "a is not b", "a not in b", "a == b")
        args = ((1, [1]), (2, [1]), (1, 1), (1, 2), (1, {1}), ("a", "ab"))
        for test in tests:
            for condition in (test, f"not ({test})"):
                f = make(f"def f(a, b):\n    if {condition}:\n        return 1\n"
                         "    return 2\n")
                jump = self.jump_of(f)
                for a in args:
                    try:
                        want = bool(eval(condition, {"a": a[0], "b": a[1]}))
                    except TypeError:
                        continue
                    # Warm it, so that the word is a specialised arm, then
                    # watch the same word
                    warm(f, *a, n=60)
                    for _ in range(2):
                        self.events.clear()
                        self.watch(f)
                        result = f(*a)
                        MON.set_local_events(TOOL, f.__code__, 0)
                        with self.subTest(condition=condition, args=a):
                            self.assertEqual(result, 1 if want else 2)
                            # The jump tests the answer of the test, whichever
                            # sense the compiler gave it
                            answer = bool(eval(test, {"a": a[0], "b": a[1]}))
                            taken = answer == (jump.opname == "POP_JUMP_IF_TRUE")
                            self.assertEqual(
                                [(e[0], e[1]) for e in self.events],
                                [("BRANCH_RIGHT" if taken else "BRANCH_LEFT", jump.offset)])

    def test_monitoring_a_warm_site_keeps_its_answers(self):
        f = make("def f(a, b):\n    if a in b:\n        return 1\n    return 2\n")
        warm(f, 1, {1})
        self.watch(f)
        self.assertEqual(f(1, {1}), 1)
        self.assertEqual(f(2, {1}), 2)
        MON.set_local_events(TOOL, f.__code__, 0)
        self.assertEqual(f(1, {1}), 1)
        self.assertEqual(f(2, {1}), 2)


if __name__ == "__main__":
    unittest.main()
