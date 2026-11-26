"""Normal send and throw completion retire their input windows."""

from pathlib import Path
import tempfile
import types
import unittest

import _testinternalcapi
from test.test_ir_call_roles import routes


class SendTransferTests(unittest.TestCase):
    def test_delegation_and_recovered_throw_reenter_the_outer_loop(self):
        def outer(fn, groups):
            for group in groups:
                yield from group
                fn(17)

        class Recover:
            def __iter__(self):
                return self

            def __next__(self):
                return 23

            def throw(self, error):
                raise StopIteration("recovered")

        # The words name the arms of the families
        names = {row[0] for row in _testinternalcapi.get_ir(outer.__code__)}
        for family in ("Send.", "CleanupThrow."):
            self.assertTrue(any(name.startswith(family) for name in names), family)
        for admitted in routes(outer.__code__):
            function = types.FunctionType(admitted, globals())
            seen = []
            groups = ((1, 2), (value for value in (3, 4)))
            self.assertEqual(list(function(seen.append, groups)), [1, 2, 3, 4])
            self.assertEqual(seen, [17, 17])
            seen.clear()
            generator = function(seen.append, (Recover(), (42,)))
            self.assertEqual(next(generator), 23)
            self.assertEqual(generator.throw(ValueError()), 42)
            self.assertEqual(list(generator), [])
            self.assertEqual(seen, [17, 17])

    def test_completed_send_leaves_its_sent_value_slot_empty(self):
        # A send that completes retires its whole window.  The window of an
        # inner async for reuses the slots of the outer one, and a stale sent
        # value in the inner cursor slot is read as a cursor.
        async def count(n):
            for i in range(n):
                yield i

        async def nested(outer, inner):
            seen = []
            async for x in count(outer):
                async for y in count(inner):
                    seen.append((x, y))
            return seen

        def run(coroutine):
            try:
                while True:
                    coroutine.send(None)
            except StopIteration as stop:
                return stop.value

        self.assertEqual(run(nested(2, 2)), [(0, 0), (0, 1), (1, 0), (1, 1)])
        self.assertEqual(run(nested(1, 0)), [])


if __name__ == "__main__":
    unittest.main()
