"""Tests for the tracing verifier (Python/gc_trace.c).

After a full collection the verifier marks everything reachable from the
roots and reports each live object that it did not reach: a "hole", a
reference that a tracing collector would miss.  These tests hold an object
only in one kind of root at a time and check that it is not a hole.
"""

import gc
import sys
import textwrap
import threading
import time
import unittest
import weakref
from test import support
from test.support import import_helper, script_helper, threading_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")

if (not hasattr(_testinternalcapi, "gc_verify")
        or _testinternalcapi.gc_verify() is None):
    raise unittest.SkipTest("the tracing verifier cannot run in this build")


class Obj:
    pass


def make_obj():
    return Obj()


def make_str():
    return "".join(["a string ", "only an array holds"])


def make_cycle():
    a = Obj()
    b = Obj()
    a.other = b
    b.other = a
    return a


# A module global, set by test_module_global.
held_in_global = None


class VerifierTests(unittest.TestCase):

    def check_no_holes(self, holes, addr=None):
        """Check that the verification found no holes, and in particular
        that the object at addr is not one."""
        self.assertIsNotNone(holes)
        if addr is not None:
            self.assertNotIn(addr, [h[1] for h in holes])
        self.assertEqual(holes, [])

    def verify_held(self, addr, ref):
        # The object at addr is referenced only from the root under test.
        holes = _testinternalcapi.gc_verify()
        self.check_no_holes(holes, addr)
        self.assertIsNotNone(ref())

    def test_c_local(self):
        holes, addr = _testinternalcapi.gc_verify_with_c_local(make_obj)
        self.check_no_holes(holes, addr)

    def test_frame_slot(self):
        x = Obj()
        addr, ref = id(x), weakref.ref(x)
        self.verify_held(addr, ref)
        self.assertIs(ref(), x)

    def test_generator_slots(self):
        def gen():
            x = Obj()
            yield id(x), weakref.ref(x)
            yield x

        g = gen()
        addr, ref = next(g)
        self.verify_held(addr, ref)
        self.assertIs(next(g), ref())

    def test_static_type_dict(self):
        # The descriptor is referenced only from the static type's dict.
        addr = id(vars(int)["bit_length"])
        holes = _testinternalcapi.gc_verify()
        self.check_no_holes(holes, addr)

    def test_module_global(self):
        global held_in_global
        held_in_global = Obj()
        try:
            addr, ref = id(held_in_global), weakref.ref(held_in_global)
            self.verify_held(addr, ref)
        finally:
            held_in_global = None

    def test_weakref_callback(self):
        target = Obj()
        callback = lambda wr: None
        wr = weakref.ref(target, callback)
        addr, ref = id(callback), weakref.ref(callback)
        del callback
        self.verify_held(addr, ref)
        self.assertIs(wr.__callback__, ref())

    def test_closure_cell(self):
        def outer():
            x = Obj()
            def inner():
                return x
            return inner, id(x), weakref.ref(x)

        inner, addr, ref = outer()
        self.verify_held(addr, ref)
        self.assertIs(inner(), ref())

    @threading_helper.requires_working_threading()
    def test_other_thread_stack(self):
        # The object is held in a C local of a thread that waits on a lock,
        # with the GIL released, while this thread verifies.
        lock = threading.Lock()
        lock.acquire()
        started = threading.Event()
        result = {}

        def wait(addr):
            result["addr"] = addr
            started.set()
            with lock:
                pass
            return addr

        def run():
            result["ret"] = _testinternalcapi.call_with_c_local(make_obj, wait)

        t = threading.Thread(target=run)
        t.start()
        try:
            started.wait()
            # Let the thread block in lock.acquire(); if it has not yet, it
            # waits for the GIL instead, which also saves its registers.
            time.sleep(0.05)
            holes = _testinternalcapi.gc_verify()
        finally:
            lock.release()
            t.join()
        self.check_no_holes(holes, result["addr"])
        self.assertEqual(result["ret"], result["addr"])

    def test_extension_static_variable(self):
        _testcapi = import_helper.import_module("_testcapi")
        x = Obj()
        addr, ref = id(x), weakref.ref(x)
        _testcapi.set_static_object(x)
        del x
        try:
            self.verify_held(addr, ref)
        finally:
            _testcapi.set_static_object(None)
        support.gc_collect()  # For other GCs.
        self.assertIsNone(ref())

    def test_cycle_held_by_c_reference(self):
        # The collection must not free the cycle, and the verifier must find
        # both members through the C local.
        holes, addr = _testinternalcapi.gc_verify_with_c_local(make_cycle)
        self.check_no_holes(holes, addr)

    def verify_pymem_array(self):
        # The conservative scan can find a stale copy of the object's
        # address, left by an earlier object at the same address, in a live
        # C stack frame.  Keep each object, so that every attempt uses a new
        # address.
        kept = []
        for attempt in range(5):
            holes, obj = _testinternalcapi.gc_verify_with_pymem_array(make_str)
            kept.append(obj)
            if [h[1] for h in holes] == [id(obj)]:
                return holes
        self.fail(f"the object is not reported as a hole: {holes}")

    @support.requires_refcounting
    def test_pymem_array_is_a_hole(self):
        # An object referenced only from PyMem_Malloc() memory is invisible
        # to a tracing collector, and the verifier must say so.  Ports must
        # fix this pattern: give the owning object a tp_traverse that visits
        # the array, or register the array with _PyGC_AddRoot().
        #
        # The object is not a GC object: the cycle collector, which runs
        # just before the verifier, would leave the addresses of the GC
        # objects it visits in its stack frame.
        holes = self.verify_pymem_array()
        name, hole_addr, refcnt, referrer = holes[0]
        self.assertEqual(name, "str")
        self.assertEqual(refcnt, 1)
        self.assertRegex(referrer, r"^non-object block at 0x[0-9a-f]+ "
                                   r"\(\d+ bytes\)$")

    @support.requires_refcounting
    def test_hole_list_is_kept(self):
        holes = self.verify_pymem_array()
        self.assertEqual(_testinternalcapi.gc_verify_holes(), holes)
        self.check_no_holes(_testinternalcapi.gc_verify())
        self.assertEqual(_testinternalcapi.gc_verify_holes(), [])


@unittest.skipUnless(support.Py_DEBUG or support.with_mimalloc(),
                     "requires the mimalloc allocator")
class ModeTests(unittest.TestCase):

    def count_verifications(self, code, *args, **env):
        """Run code in a child process and return the number of
        verifications that ran, and its stderr."""
        env.setdefault("PYTHON_GCVERIFY", "")
        code = ("import gc, _testinternalcapi\n" + code +
                "\nprint(_testinternalcapi.gc_verify_count())")
        _, out, err = script_helper.assert_python_ok(
            *args, "-c", code, __isolated=False, **env)
        return int(out), err

    def test_environment_variable(self):
        self.assertEqual(self.count_verifications(
            "gc.collect()", PYTHON_GCVERIFY="1")[0], 1)
        self.assertEqual(self.count_verifications(
            "gc.collect()", PYTHON_GCVERIFY="0")[0], 0)

    def test_xoption(self):
        self.assertEqual(self.count_verifications(
            "gc.collect()", "-X", "gcverify", PYTHON_GCVERIFY="0")[0], 1)
        self.assertEqual(self.count_verifications(
            "gc.collect()", "-X", "gcverify=0", PYTHON_GCVERIFY="1")[0], 0)

    def test_default(self):
        # On in debug builds, off in release builds.
        count, _ = self.count_verifications("gc.collect()")
        self.assertEqual(count, 1 if support.Py_DEBUG else 0)

    # Enough live allocations for several collections of generation 1.
    ALLOCATE = "gc.enable(); keep = [[] for i in range(100_000)]"

    @unittest.skipIf(support.Py_GC, "Py_GC has one generation")
    def test_explicit_collections_only(self):
        # Without "auto", only explicit full collections are verified.
        count, _ = self.count_verifications(
            "gc.collect(0); gc.collect(1)\n" + self.ALLOCATE,
            PYTHON_GCVERIFY="1")
        self.assertEqual(count, 0)

    @unittest.skipIf(support.Py_GC, "Py_GC has one generation")
    def test_auto(self):
        code = (self.ALLOCATE + "\nassert gc.get_stats()[1]['collections']")
        count, _ = self.count_verifications(code, PYTHON_GCVERIFY="auto")
        self.assertGreater(count, 0)

    @support.requires_refcounting
    def test_report(self):
        # Holes go to stderr.
        code = textwrap.dedent("""
            kept = [_testinternalcapi.gc_verify_with_pymem_array(
                        lambda: "".join(["only an array ", "holds this"]))
                    for i in range(5)]
        """)
        _, err = self.count_verifications(code, PYTHON_GCVERIFY="0")
        self.assertRegex(err, rb"gc verify: 1 holes; \d+ objects")
        self.assertRegex(err, rb"gc verify: hole str 'only an array holds "
                              rb"this' at 0x[0-9a-f]+ refcnt 1; referrer: "
                              rb"non-object block at 0x[0-9a-f]+")

    def test_behaviour_unchanged(self):
        code = textwrap.dedent("""
            import gc
            class C:
                def __del__(self):
                    pass
            for i in range(100):
                a = C(); a.a = a
            del a
            print(gc.collect(), len(gc.garbage), gc.get_stats()[2]['collected'])
        """)
        outs = []
        for mode in ("0", "1"):
            _, out, err = script_helper.assert_python_ok(
                "-c", code, PYTHON_GCVERIFY=mode)
            self.assertEqual(err, b"")
            outs.append(out)
        self.assertEqual(outs[0], outs[1])

    @unittest.skipIf(support.Py_GC, "Py_GC always uses mimalloc")
    def test_other_allocator(self):
        # The command line option is an error with another allocator; the
        # environment variable is ignored, so that child processes started
        # with another PYTHONMALLOC still run.
        rc, _, err = script_helper.assert_python_failure(
            "-X", "gcverify", "-c", "pass", PYTHONMALLOC="malloc")
        self.assertIn(b"-X gcverify requires the mimalloc allocator", err)
        count, _ = self.count_verifications(
            "gc.collect()", PYTHONMALLOC="malloc", PYTHON_GCVERIFY="1")
        self.assertEqual(count, 0)


if __name__ == "__main__":
    unittest.main()
