"""The Truth, BranchTruth and BranchNone families (Python/register_ops/truth.rop)
on whole functions, against what the language says the truth of the operand
is."""

import signal
import unittest

from test.support import import_helper

capi = import_helper.import_module("_testinternalcapi")

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
