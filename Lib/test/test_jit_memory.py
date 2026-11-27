"""Tests of the JIT's code memory (Include/internal/pycore_jit_memory.h,
Python/jit_memory.c), by the hooks in Modules/_testinternalcapi/jit_memory.c.

Code is placed as bytes: a program that returns n is `mov rax, n; ret`.
"""

import os
import re
import struct
import sys
import unittest
from test import support
from test.support import import_helper, warnings_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")
if not hasattr(_testinternalcapi, "jit_memory_place"):
    raise unittest.SkipTest("the JIT code memory is not supported on this build")

from _testinternalcapi import (
    jit_memory_info, jit_memory_alloc, jit_memory_free, jit_memory_place,
    jit_memory_call, jit_memory_text_address)


def returns(n):
    "Machine code of `mov rax, n; ret`"
    return b"\x48\xc7\xc0" + struct.pack("<i", n) + b"\xc3"


def place(code):
    return jit_memory_place(code)


def maps_entry(address):
    """The permissions of the mapping that contains the address."""
    with open("/proc/self/maps") as f:
        for line in f:
            match = re.match(r"([0-9a-f]+)-([0-9a-f]+) (\S+) ", line)
            low, high = int(match[1], 16), int(match[2], 16)
            if low <= address < high:
                return match[3]
    return None


@unittest.skipUnless(os.path.exists("/proc/self/maps"), "needs /proc")
@unittest.skipIf(sys._jit.is_enabled(),
                 "the JIT places code as the test counts pages")
class MemoryTests(unittest.TestCase):

    def setUp(self):
        self.info = jit_memory_info()
        self.assertIsNotNone(self.info)
        self.before = self.info["pages_in_use"]

    def test_reservation_is_within_reach_of_the_text(self):
        info = self.info
        low = min(info["reservation"], info["text_low"])
        high = max(info["reservation"] + info["reservation_size"], info["text_high"])
        self.assertLess(high - low, 2**31)
        function = jit_memory_text_address()
        self.assertTrue(info["text_low"] <= function < info["text_high"])

    def test_placed_code_runs(self):
        address, size = place(returns(7))
        try:
            self.assertEqual(jit_memory_call(address), 7)
        finally:
            jit_memory_free(address, size)

    def test_placed_pages_are_not_writable(self):
        address, size = place(returns(1))
        try:
            self.assertEqual(maps_entry(address)[:3], "r-x")
            self.assertTrue(address % self.info["page_size"] == 0)
        finally:
            jit_memory_free(address, size)
        self.assertEqual(maps_entry(address)[:3], "---")

    def test_no_page_is_writable_and_executable(self):
        placed = [place(returns(0)) for _ in range(8)]
        raw = jit_memory_alloc(10000)         # filled, not yet sealed
        try:
            self.assertEqual(maps_entry(raw)[:3], "rw-")
            info = self.info
            end = info["reservation"] + info["reservation_size"]
            with open("/proc/self/maps") as f:
                for line in f:
                    match = re.match(r"([0-9a-f]+)-([0-9a-f]+) (\S+) ", line)
                    low = int(match[1], 16)
                    if info["reservation"] <= low < end:
                        self.assertNotRegex(match[3], r"^.wx")
        finally:
            jit_memory_free(raw, 10000)
            for address, size in placed:
                jit_memory_free(address, size)

    def test_many_blocks(self):
        blocks = [place(returns(i)) for i in range(1000)]
        self.assertEqual(len({address for address, _ in blocks}), 1000)
        after = jit_memory_info()
        self.assertEqual(after["pages_in_use"], self.before + 1000)
        self.assertEqual(after["bytes_in_use"] % 8, 0)
        for i, (address, size) in enumerate(blocks):
            self.assertEqual(jit_memory_call(address), i)
        # Free every other block, then place again: freed pages are reused.
        for address, size in blocks[::2]:
            jit_memory_free(address, size)
        self.assertEqual(jit_memory_info()["pages_in_use"], self.before + 500)
        again = [place(returns(7)) for _ in range(500)]
        self.assertEqual(jit_memory_info()["pages_in_use"], self.before + 1000)
        self.assertEqual({jit_memory_call(a) for a, _ in again}, {7})
        for address, size in again + blocks[1::2]:
            jit_memory_free(address, size)
        self.assertEqual(jit_memory_info()["pages_in_use"], self.before)

    def test_large_block_spans_pages(self):
        size = 3 * self.info["page_size"] + 1
        address = jit_memory_alloc(size)
        try:
            self.assertEqual(jit_memory_info()["pages_in_use"], self.before + 4)
        finally:
            jit_memory_free(address, size)
        self.assertEqual(jit_memory_info()["pages_in_use"], self.before)

    def test_exhausted_reservation_is_an_error(self):
        chunk = 64 << 20
        blocks = []
        while (address := jit_memory_alloc(chunk)) is not None:
            blocks.append(address)
            self.assertLessEqual(len(blocks), self.info["reservation_size"] // chunk)
        try:
            self.assertGreater(len(blocks), 0)
            self.assertIsNone(place(returns(0)))
            self.assertIsNone(jit_memory_alloc(chunk))
        finally:
            for address in blocks:
                jit_memory_free(address, chunk)
        placed = place(returns(3))
        try:
            self.assertEqual(jit_memory_call(placed[0]), 3)
        finally:
            jit_memory_free(*placed)

    def test_unusable_requests_are_errors(self):
        self.assertIsNone(jit_memory_alloc(2**60))
        self.assertIsNone(jit_memory_alloc(0))
        self.assertIsNone(place(b""))
        self.assertEqual(jit_memory_info()["pages_in_use"], self.before)

    @support.requires_fork()
    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_fork_keeps_placed_code_and_places_its_own(self):
        placed = place(returns(11))
        pid = os.fork()
        if pid == 0:
            own = place(returns(12))
            ok = jit_memory_call(placed[0]) == 11 and jit_memory_call(own[0]) == 12
            os._exit(0 if ok else 1)
        _, status = os.waitpid(pid, 0)
        jit_memory_free(*placed)
        self.assertEqual(os.waitstatus_to_exitcode(status), 0)


if __name__ == "__main__":
    unittest.main()
