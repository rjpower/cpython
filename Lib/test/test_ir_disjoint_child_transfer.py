"""Normal attribute transport through independently reserved Python children."""

import marshal
import sys
import types
import unittest

import _testinternalcapi
from test.test_ir_admission import construct
from test.test_ir_transfer_admission import encoded


def routes(code):
    return code, construct(code), code.replace(), marshal.loads(marshal.dumps(code))


def target(value=17, **kwargs):
    return value, kwargs


def identity(value):
    return value


class Plain:
    call = staticmethod(target)


class Property:
    @property
    def call(self):
        return target


class Overridden:
    def __getattribute__(self, name):
        if name == 'call':
            return target
        return object.__getattribute__(self, name)


class DisjointChildTransferTests(unittest.TestCase):
    def test_local_recipient_consumes_owner_across_branch_join(self):
        def invoke(owner, branch, value):
            if branch:
                function = identity(owner).call
            else:
                function = target
            return function(value)

        rows = _testinternalcapi.get_ir(invoke.__code__)
        attrs = [row for row in rows if row[0] == 'LOAD_ATTR']
        self.assertEqual(len(attrs), 1)
        self.assertLess(attrs[0][1], invoke.__code__.co_nlocals)
        self.assertGreaterEqual(attrs[0][2], invoke.__code__.co_nlocals)
        for admitted in routes(invoke.__code__):
            function = types.FunctionType(admitted, globals())
            for owner in (Plain(), Property(), Overridden()):
                for branch in (False, True):
                    with self.subTest(owner=type(owner), branch=branch):
                        for _ in range(100):
                            self.assertEqual(function(owner, branch, 42), (42, {}))

    def test_sibling_call_hole_survives_child_and_alias_result_does_not(self):
        def invoke(owner, args, kwargs):
            return identity(owner).call(*args, **kwargs)

        rows = [tuple(row) for row in _testinternalcapi.get_ir(invoke.__code__)]
        site = next(i for i, row in enumerate(rows) if row[0] == 'LOAD_ATTR')
        name, dst, owner, feedback, arg = rows[site]
        self.assertFalse(arg & 1)
        self.assertTrue(any(row[0] == 'CALL_EX_NO_SELF_KW' for row in rows))
        for admitted in routes(invoke.__code__):
            function = types.FunctionType(admitted, globals())
            for obj in (Plain(), Property(), Overridden()):
                for _ in range(100):
                    self.assertEqual(function(obj, (42,), {'answer': 17}),
                                     (42, {'answer': 17}))
        # Publishing a real result into the later virtual self home is not
        # a consumed-input or untouched-prefix proof for that same home.
        rows[site] = name, owner + 1, owner, feedback, arg
        changed = encoded(rows)
        for route in (construct, lambda co, **kw: co.replace(**kw)):
            with self.assertRaisesRegex(ValueError, 'virtual call input'):
                route(invoke.__code__, co_code=changed)
        payload = marshal.dumps(invoke.__code__)
        self.assertEqual(payload.count(invoke.__code__.co_code), 1)
        with self.assertRaisesRegex(ValueError, 'virtual call input'):
            marshal.loads(payload.replace(invoke.__code__.co_code, changed, 1))

    def test_child_callback_updates_current_parent_bindings(self):
        class Mutating:
            @property
            def call(self):
                frame = sys._getframe(1)
                frame.f_locals['value'] = 42
                return target

        def invoke(owner, value):
            function = identity(owner).call
            return function(value)

        for admitted in routes(invoke.__code__):
            function = types.FunctionType(admitted, globals())
            for _ in range(100):
                self.assertEqual(function(Mutating(), 17), (42, {}))

    def test_raising_child_follows_exception_route(self):
        class Raising:
            @property
            def call(self):
                raise LookupError('child')

        def invoke(owner):
            try:
                function = identity(owner).call
            except LookupError:
                function = target
            return function(42)

        for admitted in routes(invoke.__code__):
            function = types.FunctionType(admitted, globals())
            for _ in range(100):
                self.assertEqual(function(Raising()), (42, {}))


if __name__ == '__main__':
    unittest.main()
