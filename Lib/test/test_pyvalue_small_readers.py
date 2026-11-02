"""Canonical tuple consumers in byte matching and tokenizer decoder state."""

import unittest


class ByteTupleControls(unittest.TestCase):
    def test_first_match_and_empty_tuple(self):
        for receiver in (b"abc", bytearray(b"abc")):
            for method, match, miss in ((receiver.startswith, b"a", b"z"),
                                        (receiver.endswith, b"c", b"z")):
                with self.subTest(receiver=type(receiver), method=method):
                    self.assertFalse(method(()))
                    self.assertFalse(method((miss,)))
                    self.assertTrue(method((miss, match)))
                    self.assertTrue(method((match, 2147483647)))
                    self.assertTrue(method((match, 3.25)))

    def test_invalid_candidate_order_and_type(self):
        for receiver in (b"abc", bytearray(b"abc")):
            for method in (receiver.startswith, receiver.endswith):
                for value in (2147483647, 3.25, float("nan"), 1 << 100,
                              None, True, "a"):
                    with self.subTest(receiver=type(receiver), value=value):
                        with self.assertRaisesRegex(TypeError, "bytes-like object"):
                            method((value, b"abc"))
                with self.assertRaisesRegex(TypeError, "first arg must be bytes"):
                    method(2147483647)

    def test_bytes_subclasses_and_buffer_candidates(self):
        class Bytes(bytes):
            pass

        for receiver in (b"abc", bytearray(b"abc")):
            self.assertTrue(receiver.startswith((Bytes(b"a"),)))
            self.assertTrue(receiver.endswith((memoryview(b"c"),)))
            self.assertTrue(receiver.startswith((bytearray(b"a"),)))
            self.assertFalse(receiver.endswith((memoryview(b"x"),)))

    def test_offsets_and_short_circuit(self):
        for receiver in (b"xabcx", bytearray(b"xabcx")):
            self.assertTrue(receiver.startswith((b"a", None), 1, 4))
            self.assertTrue(receiver.endswith((b"c", None), 1, 4))
            self.assertFalse(receiver.startswith((b"a",), 0, 0))
            self.assertTrue(receiver.endswith((b"",), 1, 4))


def decoder_pending(decoder):
    import _testinternalcapi
    # This route uses the repaired expanded native-call gateway.
    return _testinternalcapi.tokenizer_decoder_buffered_input(*(decoder,))


class DecoderCandidateTests(unittest.TestCase):
    """Calls the actual candidate decoder provider through its registered fixture."""

    def test_valid_inline_heap_and_bool_flags(self):
        class Decoder:
            calls = 0

            def getstate(self):
                self.calls += 1
                return self.state

        decoder = Decoder()
        flags = (0, 2147483647, 1 << 100, True, False)
        for flag in flags:
            for value in (b"", b"pending"):
                decoder.state = (value, flag)
                with self.subTest(flag=flag, value=value):
                    self.assertEqual(decoder_pending(decoder), bool(value))
        self.assertEqual(decoder.calls, len(flags) * 2)

    def test_tuple_bytes_and_int_subclasses(self):
        class State(tuple):
            def __getitem__(self, index):
                raise AssertionError("protocol must read tuple storage")

        class Bytes(bytes):
            pass

        class Flag(int):
            pass

        class Decoder:
            def getstate(self):
                return State((Bytes(b"pending"), Flag(1)))

        self.assertEqual(decoder_pending(Decoder()), 1)

    def test_malformed_state(self):
        class Decoder:
            def getstate(self):
                return self.state

        decoder = Decoder()
        for state in (None, [], (b"",), (b"", 0, None), ("", 0),
                      (2147483647, 0), (b"", 3.25), (b"", None)):
            decoder.state = state
            with self.subTest(state=state), self.assertRaisesRegex(
                    TypeError, r"getstate\(\) must return \(bytes, int\)"):
                decoder_pending(decoder)

    def test_getstate_error_identity(self):
        class Decoder:
            def getstate(self):
                raise self.error

        decoder = Decoder()
        for error in (MemoryError("decode memory"), RuntimeError("decode primary")):
            decoder.error = error
            with self.assertRaises(type(error)) as caught:
                decoder_pending(decoder)
            self.assertIs(caught.exception, error)

    def test_state_owner_retires_once(self):
        events = []

        class State(tuple):
            def __del__(self):
                events.append("state retired")

        class Decoder:
            def getstate(self):
                return State((b"pending", 2147483647))

        self.assertEqual(decoder_pending(Decoder()), 1)
        self.assertEqual(events, ["state retired"])


if __name__ == "__main__":
    unittest.main()
