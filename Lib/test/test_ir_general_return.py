"""General returns transfer remaining scratch owners to frame retirement."""

import marshal
import sys
import types
import unittest
import weakref

import _testinternalcapi
from test import support
from test.test_ir_admission import construct
from test.test_ir_transfer_admission import encoded, make_code


def routes(code):
    return (code, construct(code), code.replace(),
            marshal.loads(marshal.dumps(code)))


def general_code():
    return make_code([
        ("LOAD_FAST_AND_CLEAR", 2, 0),
        ("LOAD_FAST_AND_CLEAR", 3, 1),
        ("IR_JUMP", 0, 0, 3),
        ("RETURN_VALUE", 3)],
        locals=("owner", "result"), slots=2).replace(co_argcount=2)


class GeneralReturnTests(unittest.TestCase):
    def test_remaining_owner_is_valid_but_not_an_empty_return(self):
        code = general_code()
        for admitted in routes(code):
            self.assertEqual(_testinternalcapi.get_ir(admitted)[-1][0],
                             "RETURN_VALUE")
            self.assertEqual(_testinternalcapi.check_ir_retirement(
                admitted, 3, 1), 0)

    def test_return_input_and_overwrite_checks_remain_required(self):
        code = general_code()
        controls = (
            ([("LOAD_FAST_AND_CLEAR", 2, 0), ("IR_JUMP", 0, 0, 2),
              ("IR_JUMP", 0, 0, 3), ("RETURN_VALUE", 3)],
             "reads unavailable scratch"),
            ([("LOAD_FAST_AND_CLEAR", 2, 0),
              ("LOAD_FAST_AND_CLEAR", 3, 1), ("LOAD_CONST", 2),
              ("RETURN_VALUE", 3)], "overwrites scratch owner"),
        )
        for words, message in controls:
            changed = encoded(words)
            for route in (construct, lambda co, **kw: co.replace(**kw)):
                with self.subTest(message=message, route=route):
                    with self.assertRaisesRegex(ValueError, message):
                        route(code, co_code=changed)
            payload = marshal.dumps(code)
            self.assertEqual(payload.count(code.co_code), 1)
            with self.assertRaisesRegex(ValueError, message):
                marshal.loads(payload.replace(code.co_code, changed, 1))

    def test_last_scratch_owner_keeps_result_and_code_rooted(self):
        for code in routes(general_code()):
            events = []
            observations = []
            result_ids = []

            class Result:
                def __init__(self):
                    result_ids.append(id(self))

                def __del__(self):
                    events.append("result")

            class Owner:
                def __del__(self):
                    support.gc_collect()
                    roots = set(_testinternalcapi.get_transition_root_ids())
                    states = _testinternalcapi.get_transition_shadow_status()
                    observations.append((result_ids[0] in roots,
                                         id(code) in roots,
                                         [row for row in states if row[0] == 1]))
                    events.append("owner")

            function = types.FunctionType(code, {})
            result = function(Owner(), Result())
            self.assertEqual(events, ["owner"])
            self.assertEqual(observations,
                             [(True, True, [(1, 1, 0, True)])])
            self.assertEqual(id(result), result_ids[0])
            self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])
            del result
            support.gc_collect()
            self.assertEqual(events, ["owner", "result"])

    def test_association_takeover_preserves_remaining_scratch(self):
        # The callback escapes the actual returning frame. Its scratch owner
        # must move into that frame's heap storage rather than disappear at L.
        code = make_code([
            ("LOAD_FAST_AND_CLEAR", 4, 0),
            ("LOAD_FAST_AND_CLEAR", 5, 1),
            ("LOAD_FAST", 6, 2),
            ("CALL_NO_SELF", 3, 6, 0, 0),
            ("RETURN_VALUE", 5)],
            locals=("owner", "result", "observe", "unused"),
            slots=4).replace(co_argcount=3)
        for admitted in routes(code):
            frames = []
            owner_refs = []
            events = []

            class Owner:
                def __init__(self):
                    owner_refs.append(weakref.ref(self))

                def __del__(self):
                    events.append("owner")

            def observe():
                frames.append(sys._getframe(1))

            function = types.FunctionType(admitted, {})
            result = object()
            self.assertIs(function(Owner(), result, observe), result)
            self.assertEqual(events, [])
            self.assertIsNotNone(owner_refs[0]())
            self.assertEqual(len(frames), 1)
            self.assertIs(frames[0].f_code, admitted)
            self.assertEqual(_testinternalcapi.get_transition_root_ids(), [])
            frames[0].clear()
            support.gc_collect()
            self.assertEqual(events, ["owner"])
            self.assertIsNone(owner_refs[0]())


if __name__ == "__main__":
    unittest.main()
