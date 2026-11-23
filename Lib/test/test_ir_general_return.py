"""General returns transfer remaining owners to frame retirement.

A function that returns a value while its parameters still own objects
retires those owners after the result is out.
"""

import marshal
import sys
import types
import unittest
import weakref

from test import support


def general(owner, result):
    return result


def escaping(owner, result, observe):
    observe()
    return result


def routes(function):
    code = function.__code__
    return (code, code.replace(), marshal.loads(marshal.dumps(code)))


class GeneralReturnTests(unittest.TestCase):
    def test_last_owner_finalizer_sees_result_and_code(self):
        for code in routes(general):
            events = []
            observations = []
            result_refs = []
            code_ref = weakref.ref(code)

            class Result:
                def __init__(self):
                    result_refs.append(weakref.ref(self))

                def __del__(self):
                    events.append("result")

            class Owner:
                def __del__(self):
                    support.gc_collect()
                    observations.append((result_refs[0]() is not None,
                                         code_ref() is not None))
                    events.append("owner")

            function = types.FunctionType(code, {})
            result = function(Owner(), Result())
            self.assertEqual(events, ["owner"])
            self.assertEqual(observations, [(True, True)])
            self.assertIs(result, result_refs[0]())
            del result
            support.gc_collect()
            self.assertEqual(events, ["owner", "result"])

    def test_frame_takeover_preserves_remaining_owner(self):
        # The callback escapes the returning frame. Its owner must move into
        # that frame's heap storage rather than disappear at the return.
        for admitted in routes(escaping):
            frames = []
            owner_refs = []
            events = []

            class Owner:
                def __init__(self):
                    owner_refs.append(weakref.ref(self))

                def __del__(self):
                    events.append("owner")

            def observe():
                frame = sys._getframe(1)
                self.assertIs(frame.f_code, admitted)
                frames.append(frame)

            function = types.FunctionType(admitted, {})
            result = object()
            self.assertIs(function(Owner(), result, observe), result)
            self.assertEqual(events, [])
            self.assertIsNotNone(owner_refs[0]())
            self.assertEqual(len(frames), 1)
            self.assertIs(frames[0].f_code, admitted)
            frames[0].clear()
            support.gc_collect()
            self.assertEqual(events, ["owner"])
            self.assertIsNone(owner_refs[0]())


if __name__ == "__main__":
    unittest.main()
