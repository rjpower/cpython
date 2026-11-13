"""Reached callback shapes derived from the same events as admission."""

import marshal
import types
import unittest

import _testinternalcapi
from test.test_ir_admission import construct
from test.test_ir_transfer_admission import encoded


PUBLISHED = 1 << 8
STATUS_SHIFT = 9
UNCOVERED, EXACT, MISMATCH = range(3)
PROVIDER, RETIRE, RESTORE = 5, 6, 7


def binary_code(*, slots=4, move=True, unknown=False):
    def template(left, right):
        return left + right

    code = template.__code__
    rows = _testinternalcapi.get_ir(code)
    binary = [row for row in rows if row[0].startswith("BINARY_OP")]
    assert len(binary) == 1
    assert binary[0][0] == "BINARY_OP_ADD"
    resume = [row for row in rows if row[0] == "RESUME"]
    assert len(resume) == 1
    # A real language value survives below the two operands. Keep it in a
    # scratch home rather than a hidden constant binding or native snapshot.
    words = [tuple(resume[0]), ("LOAD_CONST", 2)]
    if unknown:
        # A real admitted operation whose effects are not yet certified.
        # It reads/replaces the initialized survivor, rather than an empty home.
        words.append(("UNARY_NOT", 2, 2))
    left, right = 0, 1
    if move:
        words += [("LOAD_FAST_AND_CLEAR", 3, 0),
                  ("LOAD_FAST_AND_CLEAR", 4, 1)]
        left, right = 3, 4
    destination = slots + 1
    site = len(words)
    words += [(binary[0][0], destination, left, right, binary[0][4]),
              ("POP_TOP", 2), ("RETURN_VALUE", destination)]
    changed = code.replace(co_code=encoded(words), co_stacksize=slots,
                           co_consts=(["surviving scratch owner"],),
                           co_linetable=b"", co_exceptiontable=b"")
    return changed, site, destination


def routes(code):
    return (code, construct(code), code.replace(),
            marshal.loads(marshal.dumps(code)))


class PhasePlanTests(unittest.TestCase):
    def test_full_extent_live_and_absent_shapes(self):
        for slots in (4, 64, 65, 4093):
            code, site, destination = binary_code(slots=slots)
            for admitted in routes(code):
                with self.subTest(slots=slots, code=admitted):
                    check = _testinternalcapi.check_ir_frame_phase
                    for phase in (PROVIDER, RETIRE, RESTORE):
                        self.assertEqual(check(admitted, site, PUBLISHED | phase), EXACT)
                    # The survivor is live in all three callback shapes.
                    self.assertEqual(check(admitted, site, PUBLISHED | PROVIDER,
                                           missing_home=2), MISMATCH)
                    # Taking the inputs leaves canonical NULL at their homes.
                    self.assertEqual(check(admitted, site, PUBLISHED | PROVIDER,
                                           dirty_home=3), MISMATCH)
                    self.assertEqual(check(admitted, site, PUBLISHED | RETIRE,
                                           missing_home=destination), MISMATCH)
                    self.assertEqual(check(admitted, site, PUBLISHED | RESTORE,
                                           dirty_home=destination), MISMATCH)

    def test_lookup_requires_image_site_and_published_phase(self):
        code, site, _ = binary_code()
        check = _testinternalcapi.check_ir_frame_phase
        self.assertEqual(check(code, site, PUBLISHED | PROVIDER), EXACT)
        for phase in (0, PROVIDER, PUBLISHED, PUBLISHED | 255,
                      PUBLISHED | PROVIDER | (3 << STATUS_SHIFT),
                      PUBLISHED | PROVIDER | (1 << 15)):
            with self.subTest(phase=phase):
                self.assertEqual(check(code, site, phase), UNCOVERED)
        for other in (0, site + 1, len(code.co_code) // 8):
            self.assertEqual(check(code, other, PUBLISHED | PROVIDER), UNCOVERED)
        for flag in ("unaligned", "wrong_image"):
            self.assertEqual(check(code, site, PUBLISHED | PROVIDER,
                                   **{flag: True}), UNCOVERED)

    def test_unknown_predecessor_never_acquires_callback_coverage(self):
        code, site, _ = binary_code(unknown=True)
        for admitted in routes(code):
            self.assertEqual(_testinternalcapi.check_ir_frame_phase(
                admitted, site, PUBLISHED | PROVIDER), UNCOVERED)

    def test_nullable_join_is_a_shadow_candidate_not_a_required_value(self):
        code, original, destination = binary_code(move=False)
        rows = _testinternalcapi.get_ir(code)
        joined = code.replace(co_code=encoded([
            rows[0], ("POP_JUMP_IF_NONE", 0, 0, 4),
            ("LOAD_CONST", 2), ("IR_JUMP", 0, 0, 4),
            rows[original], ("POP_TOP", 2), ("RETURN_VALUE", destination)]),
            co_linetable=b"")
        for admitted in routes(joined):
            check = _testinternalcapi.check_ir_frame_phase
            self.assertEqual(check(admitted, 4, PUBLISHED | PROVIDER), EXACT)
            # One predecessor has a survivor and the other has canonical
            # NULL. It cannot become a required nonNULL range at the join.
            with self.assertRaisesRegex(ValueError, "missing home must be required"):
                check(admitted, 4, PUBLISHED | PROVIDER, missing_home=2)

    def test_loop_backedge_loses_unconverted_family_completeness(self):
        code, site, destination = binary_code(move=False)
        rows = _testinternalcapi.get_ir(code)
        loop = code.replace(co_code=encoded([
            rows[0], rows[1], rows[site], ("POP_TOP", destination),
            ("IR_JUMP", 0, 0, 2)]), co_linetable=b"")
        # This image is admitted and inspected only. It deliberately has no
        # terminating path; the backedge cannot keep the first entry's proof.
        for admitted in routes(loop):
            self.assertEqual(_testinternalcapi.check_ir_frame_phase(
                admitted, 2, PUBLISHED | PROVIDER), UNCOVERED)

    def test_event_admission_rejects_duplicate_scratch_take(self):
        code, site, _ = binary_code()
        words = _testinternalcapi.get_ir(code)
        bad = list(words)
        handler, dst, left, right, fb = bad[site]
        self.assertNotEqual(left, right)
        bad[site] = (handler, dst, left, left, fb)
        replacement = encoded(bad)
        for route in (construct, lambda co, **kw: co.replace(**kw)):
            with self.assertRaisesRegex(ValueError, "reads unavailable scratch"):
                route(code, co_code=replacement)
        payload = marshal.dumps(code)
        self.assertEqual(payload.count(code.co_code), 1)
        with self.assertRaisesRegex(ValueError, "reads unavailable scratch"):
            marshal.loads(payload.replace(code.co_code, replacement, 1))


class ReachedPhaseTests(unittest.TestCase):
    def setUp(self):
        if _testinternalcapi.gc_verify() is None:
            self.skipTest("the tracing frame-root verifier cannot run in this build")

    def test_provider_and_operand_finalizers_use_nonempty_live_shapes(self):
        code, site, _ = binary_code()
        for admitted in routes(code):
            observations = []

            def record(label):
                # This invokes the actual collector frame-root walk. The
                # observation API below only reads the status it stored.
                holes = _testinternalcapi.gc_verify()
                states = [state for current, state in
                          _testinternalcapi.get_ir_frame_phases()
                          if current is admitted]
                observations.append((label, holes, states))

            class Operand:
                def __add__(self, other):
                    record("provider")
                    return ["result"]

                def __del__(self):
                    record("operand")

            f = types.FunctionType(admitted, {})
            self.assertEqual(f(Operand(), Operand()), ["result"])
            self.assertEqual([label for label, _, _ in observations],
                             ["provider", "operand", "operand"])
            for label, holes, states in observations:
                phase = PROVIDER if label == "provider" else RETIRE
                self.assertEqual(holes, [])
                self.assertEqual(states, [PUBLISHED | phase | (EXACT << STATUS_SHIFT)])

    def test_folded_bindings_remain_dynamic_during_error_restore(self):
        import sys
        code, site, _ = binary_code(move=False)
        observations = []
        bindings = []

        def record(label):
            holes = _testinternalcapi.gc_verify()
            states = [state for current, state in
                      _testinternalcapi.get_ir_frame_phases() if current is code]
            observations.append((label, holes, states))

        class Operand:
            def __add__(self, other):
                record("provider")
                target = sys._getframe(1)
                target.f_locals["left"] = None
                target.f_locals["right"] = None
                bindings.append((target.f_locals["left"], target.f_locals["right"]))
                record("mutated")
                # Let the native numeric provider raise after this Python
                # frame has returned, so its traceback cannot retain owners.
                return NotImplemented

            def __del__(self):
                record("operand")

        f = types.FunctionType(code, {})
        with self.assertRaisesRegex(TypeError, "unsupported operand"):
            f(Operand(), Operand())
        self.assertEqual(bindings, [(None, None)])
        self.assertEqual([label for label, _, _ in observations],
                         ["provider", "mutated", "operand", "operand"])
        for label, holes, states in observations:
            self.assertEqual(holes, [])
            if label == "operand":
                # The locals proxy deliberately retains replaced mortal
                # bindings until frame destruction, beyond the active chain.
                self.assertEqual(states, [])
            else:
                self.assertEqual(states, [PUBLISHED | PROVIDER | (EXACT << STATUS_SHIFT)])

    def test_error_restored_scratch_owners_survive_until_unwind(self):
        code, site, _ = binary_code()
        observations = []

        def record(label):
            holes = _testinternalcapi.gc_verify()
            states = [state for current, state in
                      _testinternalcapi.get_ir_frame_phases() if current is code]
            observations.append((label, holes, states))

        class Operand:
            def __add__(self, other):
                record("provider")
                return NotImplemented

            def __del__(self):
                record("operand")

        f = types.FunctionType(code, {})
        with self.assertRaisesRegex(TypeError, "unsupported operand"):
            f(Operand(), Operand())
        self.assertEqual([label for label, _, _ in observations],
                         ["provider", "operand", "operand"])
        for label, holes, states in observations:
            self.assertEqual(holes, [])
            if label == "provider":
                self.assertEqual(states, [PUBLISHED | PROVIDER | (EXACT << STATUS_SHIFT)])
            else:
                # RESTORE moves each owner home; it does not retire it. The
                # actual last-owner callbacks occur in ClearTemporaries, which
                # invalidates the active phase before clearing any scratch.
                self.assertEqual(states, [0])


if __name__ == "__main__":
    unittest.main()
