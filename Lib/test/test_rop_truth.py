"""The Truth, BranchTruth and BranchNone families (Python/register_ops/truth.rop)
by their arms, run on words of the functions below: the form and the
completion stub of each arm, forced onto the same slots, against what the
language says the truth of the operand is, and the adaptation that picks
each arm."""

import signal
import sys
import unittest

from test.support import import_helper

capi = import_helper.import_module("_testinternalcapi")
if not hasattr(capi, "jit_template_run"):
    raise unittest.SkipTest("needs the template probe")

from test.test_jit_template import slot_of, word_of

FORM, STUB = 0, 2
_PyIR_MISS = 1
_PyIR_ESCAPED = 2
_PyIR_RAISED = 4
_PyIR_BRANCH = 8
_PyIR_AGAIN = 16

# The sources: a local operand and a temporary one (a temporary is retired
# by the word, a local keeps its binding)
SOURCES = {
    "truth": {
        "locals": "def f(a):\n    return not a\n",
        "temporaries": "def f(a):\n    return not (a * 1)\n",
    },
    "branch": {
        "locals": "def f(a):\n    if a:\n        return 1\n    return 2\n",
        "temporaries": "def f(a):\n    if a * 1:\n        return 1\n    return 2\n",
    },
    # `if not a` jumps on true: the other member of BranchMode's drop modes
    "branch_true": {
        "locals": "def f(a):\n    if not a:\n        return 1\n    return 2\n",
        "temporaries": "def f(a):\n    if not (a * 1):\n        return 1\n    return 2\n",
    },
    "none": {
        "locals": "def f(a):\n    if a is None:\n        return 1\n    return 2\n",
    },
}
GENERIC = {"truth": "Truth.Generic", "branch": "BranchTruth.Generic",
           "branch_true": "BranchTruth.Generic", "none": "BranchNone.Test"}
# The BranchTruth member arm of each source's mode (`over(mode: ...)`)
MEMBER = {"branch": "BRANCH_IF_FALSE", "branch_true": "BRANCH_IF_TRUE"}


def branch_arm(family, arm):
    """The BranchTruth arm that a word of family's source runs as arm"""
    return f"BranchTruth.{arm}" if arm == "Generic" else f"BranchTruth.{arm}_{MEMBER[family]}"


class Final:
    log = []

    def __del__(self):
        Final.log.append("del")


class Slotted:
    def __init__(self, n):
        self.n = n

    def __bool__(self):
        return self.n != 0


class Sized:
    def __init__(self, n):
        self.n = n

    def __len__(self):
        return self.n


class Plain:
    pass


class Raises:
    def __bool__(self):
        raise ZeroDivisionError("bool")


class Redirect:
    """Its __bool__ returns a non-bool: TypeError, the word raises"""

    def __bool__(self):
        return 1


VALUES = [
    True, False, 0, 1, -1, 2**50, 2**60, 0.0, 1.5, float("nan"), None, "", "a", b"", b"x",
    [], [1], (), (1,), {}, {1: 2}, set(), Plain(), Slotted(0), Slotted(3), Sized(0),
    Sized(2), Final(), Raises(), Redirect(), type("S", (str,), {})(""),
]


def same(x, y):
    if isinstance(x, float) and isinstance(y, float) and x != x and y != y:
        return True
    return type(x) is type(y) and (x is y or x == y)


def truth_of(value):
    return bool(value)


def arms():
    return capi.arm_handlers()


def compiled(family, kind):
    namespace = {}
    exec(SOURCES[family][kind], namespace)
    f = namespace["f"]
    # An inline float never specialises, so the word stays on its generic arm
    f(1.5)
    index, _ = word_of(f, GENERIC[family])
    return f, index, dict(zip("ABCD", capi.get_ir(f.__code__, True)[index][1:]))


def probe(f, index, fields, field, value, mode, handler):
    slots = {slot_of(fields, field): value}
    before = sys.getrefcount(value)
    result = capi.jit_template_run(f, index, slots, mode, handler)
    return result, before - sys.getrefcount(value)


def run(family, kind, field, value, mode, handler):
    """Runs the word once under handler, after the generic arm of a stub
    that missed.  Returns what the probe returns and the count change."""
    f, index, fields = compiled(family, kind)
    result, counts = probe(f, index, fields, field, value, mode, handler)
    if mode == STUB and result[0] & _PyIR_MISS:
        result, counts = probe(f, index, fields, field, value, STUB, arms()[GENERIC[family]])
    # A word that specialises asks to run again under the arm it now names
    again = result[0] & _PyIR_AGAIN if mode == STUB else result[0] == index
    if again:
        mode = FORM
        result, counts = probe(f, index, fields, field, value, FORM, -1)
    return f, index, fields, result, counts, mode


def jumped(result, index, mode):
    """Whether the word left for its target: a stub's status says so, a
    form's next word is not the one after the word"""
    if mode == STUB:
        return bool(result[0] & _PyIR_BRANCH)
    return result[0] != index + 1


def check_truth(test, arm, kind, value, mode):
    f, index, fields, result, counts, mode = run("truth", kind, "B", value, mode,
                                                 arms()[arm])
    _, _, exc, slots = result
    try:
        want = not truth_of(value)
    except Exception as e:
        test.assertIsInstance(exc, type(e))
        return
    test.assertIsNone(exc)
    test.assertIs(slots[slot_of(fields, "A")], want)


def check_branch(test, family, arm, kind, value, mode, field):
    f, index, fields, result, counts, mode = run(family, kind, field, value, mode,
                                                 arms()[arm])
    _, _, exc, slots = result
    try:
        taken = ((value is not None) if family == "none"
                 else truth_of(value) if family == "branch_true"
                 else not truth_of(value))
    except Exception as e:
        test.assertIsInstance(exc, type(e))
        return
    test.assertIsNone(exc)
    test.assertEqual(jumped(result, index, mode), taken, (arm, kind, value, mode))
    # A local keeps its binding; a temporary is retired, which leaves its
    # slot empty on either edge
    kept = slots[slot_of(fields, field)]
    # (an inline value shows as its raw word, a NULL slot as 0)
    if kind == "locals":
        if not isinstance(value, (int, float, type(None))):
            test.assertTrue(same(kept, value), (arm, value, kept))
    else:
        test.assertTrue(kept is None or kept == 0, (arm, value, kept))


TRUTH_ARMS = ["Bool", "Int", "List", "None", "Str", "Plain", "Slot", "Generic"]
SHARED = {"Slot", "Generic"}


class TruthArmTests(unittest.TestCase):
    """Every arm on every value: a value the arm does not admit leaves it
    for the generic arm, and the result is the language's"""

    def test_truth(self):
        for arm in TRUTH_ARMS:
            for kind in SOURCES["truth"]:
                for value in VALUES:
                    for mode in (FORM, STUB):
                        if mode == STUB and arm not in SHARED:
                            continue
                        with self.subTest(arm=arm, kind=kind, value=value, mode=mode):
                            check_truth(self, f"Truth.{arm}", kind, value, mode)

    def test_branch(self):
        for family in MEMBER:
            for arm in TRUTH_ARMS:
                for kind in SOURCES[family]:
                    for value in VALUES:
                        for mode in (FORM, STUB):
                            if mode == STUB and arm not in SHARED:
                                continue
                            with self.subTest(family=family, arm=arm, kind=kind,
                                              value=value, mode=mode):
                                check_branch(self, family, branch_arm(family, arm), kind,
                                             value, mode, "A")

    def test_none(self):
        for value in VALUES:
            with self.subTest(value=value):
                check_branch(self, "none", "BranchNone.Test", "locals", value, FORM, "A")

    def test_temporary_operand_is_retired(self):
        Final.log.clear()
        for arm in ("BranchTruth.Generic", "BranchTruth.Plain_BRANCH_IF_FALSE"):
            for mode in (FORM, STUB):
                if mode == STUB and arm != "BranchTruth.Generic":
                    continue
                f, index, fields = compiled("branch", "temporaries")
                value = Final()
                slots = {slot_of(fields, "A"): value}
                del value
                result = capi.jit_template_run(f, index, slots, mode, arms()[arm])
                if result[0] == index if mode == FORM else result[0] & _PyIR_AGAIN:
                    # The word specialised and asks to run again
                    result = capi.jit_template_run(f, index, slots, FORM, -1)
                del slots
                self.assertIsNone(result[2])
                self.assertEqual(Final.log, ["del"], (arm, mode))
                Final.log.clear()


def adapted_arm(family, value):
    """The arm that a word reaches after the generic arm runs on value"""
    f, index, fields = compiled(family, "locals")
    field = "B" if family == "truth" else "A"
    generic = arms()[GENERIC[family]]
    for _ in range(4000):
        capi.jit_template_run(f, index, {slot_of(fields, field): value}, FORM, generic)
        name = capi.get_ir(f.__code__, True)[index][0]
        if name != GENERIC[family]:
            return name
        generic = -1 if name != GENERIC[family] else generic
    return GENERIC[family]


class AdaptationTests(unittest.TestCase):
    EXPECTED = [
        (True, "Bool"), (False, "Bool"), (3, "Int"), (2**60, "Int"), ([], "List"),
        ([1], "List"), (None, "None"), ("", "Str"), ("abc", "Str"), (Plain(), "Plain"),
        (Slotted(1), "Slot"), (Sized(0), "Slot"),
    ]

    def test_each_arm_is_picked(self):
        for family in ("truth", *MEMBER):
            for value, arm in self.EXPECTED:
                with self.subTest(family=family, value=value):
                    want = f"Truth.{arm}" if family == "truth" else branch_arm(family, arm)
                    self.assertEqual(adapted_arm(family, value), want)

    def test_floats_stay_generic(self):
        for family in ("truth", *MEMBER):
            self.assertEqual(adapted_arm(family, 1.5), GENERIC[family])

    def test_a_word_runs_on_after_a_miss(self):
        for family, prefix in (("truth", "Truth"), ("branch", "BranchTruth")):
            f, index, fields = compiled(family, "locals")
            field = "B" if family == "truth" else "A"
            for value in (3, [], None, "x"):
                name = adapted_arm(family, value)
                self.assertTrue(name.startswith(prefix))


class FunctionTests(unittest.TestCase):
    """Whole functions, which the translator now lowers to the families"""

    def test_conditions(self):
        def f(a):
            if a:
                return "yes"
            return "no"

        def g(a):
            return not a

        def h(a):
            return 1 if a is None else 2

        def k(a):
            while a:
                a = a[1:]
            return a

        for _ in range(3):
            for value in VALUES:
                if isinstance(value, (Raises, Redirect)):
                    continue
                self.assertEqual(f(value), "yes" if value else "no")
                self.assertEqual(g(value), not value)
                self.assertEqual(h(value), 1 if value is None else 2)
        for _ in range(3):
            self.assertEqual(k([1, 2, 3]), [])
            self.assertEqual(k("abc"), "")

    def test_failures(self):
        def f(a):
            if a:
                return 1
            return 2

        for _ in range(3):
            with self.assertRaises(ZeroDivisionError):
                f(Raises())
            with self.assertRaises(TypeError):
                f(Redirect())

    def test_far_branch(self):
        # A branch whose target is beyond a 12-bit distance
        body = "\n".join(f"    x = a + {i}" for i in range(2200))
        namespace = {}
        exec(f"def f(a):\n    if a:\n{body.replace('    x', '        x')}\n    return 7\n",
             namespace)
        f = namespace["f"]
        for _ in range(3):
            self.assertEqual(f(0), 7)
            self.assertEqual(f(1), 7)




def jump_words(f):
    """The (arm, index) of each Jump word of f"""
    return [(word[0].partition("+")[0], index)
            for index, word in enumerate(capi.get_ir(f.__code__, True))
            if word[0].startswith("Jump.")]


def distance(fields):
    """The signed 24-bit target of a Jump word, in A and B"""
    value = fields[0] | fields[1] << 12
    return value - (1 << 24) if value >= 1 << 23 else value


class JumpTests(unittest.TestCase):
    """The Jump family: a backedge polls, a forward jump does not, and the
    translator picks the arm"""

    def test_arms_follow_the_stack_op(self):
        def f(n):
            while n:
                n -= 1
                if n == 3:
                    continue
            return n

        f(5)
        arms = {arm for arm, _ in jump_words(f)}
        # With the JIT on the backedge is also its warm-up event
        self.assertTrue(arms & {"Jump.Poll", "Jump.Warm"}, arms)
        self.assertLessEqual(arms, {"Jump.Direct", "Jump.Poll", "Jump.Warm"})

    def test_forms_go_to_their_targets(self):
        def f(n):
            while n:
                n -= 1
            return n

        f(3)
        words = capi.get_ir(f.__code__, True)
        found = 0
        for arm, index in jump_words(f):
            fields = [int(x) for x in words[index][1:3]]
            target = index + 1 + distance(fields)
            for mode, handler in ((FORM, arms()[arm]),):
                with self.subTest(arm=arm, index=index):
                    result = capi.jit_template_run(f, index, {}, mode, handler)
                    self.assertEqual(result[0], target)
                    self.assertIsNone(result[2])
            found += 1
        self.assertGreater(found, 0)

    def test_a_backedge_polls(self):
        class Interrupted(Exception):
            pass

        def handler(signum, frame):
            raise Interrupted

        def spin():
            while True:
                pass

        old = signal.signal(signal.SIGALRM, handler)
        try:
            for _ in range(3):
                signal.setitimer(signal.ITIMER_REAL, 0.02)
                with self.assertRaises(Interrupted):
                    spin()
        finally:
            signal.setitimer(signal.ITIMER_REAL, 0)
            signal.signal(signal.SIGALRM, old)

    def test_loops(self):
        def f(n):
            total = 0
            for i in range(n):
                if i % 3 == 0:
                    continue
                total += i
            while total > 10:
                total -= 7
            return total

        for _ in range(3):
            self.assertEqual(f(30), 6)

    def test_yield_from_loop(self):
        def inner():
            yield 1
            yield 2
            return 5

        def outer():
            x = yield from inner()
            yield x

        for _ in range(3):
            self.assertEqual(list(outer()), [1, 2, 5])


if __name__ == "__main__":
    unittest.main()
