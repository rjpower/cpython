import unittest
import unittest.mock
from test import support
from test.support import (verbose,
                          cpython_only, requires_subprocess,
                          Py_GIL_DISABLED)
from test.support.os_helper import temp_dir, TESTFN, unlink
from test.support.script_helper import assert_python_ok, make_script
from test.support import threading_helper, gc_threshold

import gc
import sys
import textwrap
import threading
import time
import weakref

try:
    import _testcapi
    from _testcapi import with_tp_del
    from _testcapi import ContainerNoGC
except ImportError:
    _testcapi = None
    def with_tp_del(cls):
        class C(object):
            def __new__(cls, *args, **kwargs):
                raise unittest.SkipTest('requires _testcapi.with_tp_del')
        return C
    ContainerNoGC = None

try:
    import _testinternalcapi
except ImportError:
    _testinternalcapi = None

### Support code
###############################################################################

# Bug 1055820 has several tests of longstanding bugs involving weakrefs and
# cyclic gc.

# An instance of C1055820 has a self-loop, so becomes cyclic trash when
# unreachable.
class C1055820(object):
    def __init__(self, i):
        self.i = i
        self.loop = self

class GC_Detector(object):
    # Create an instance I.  Then gc hasn't happened again so long as
    # I.gc_happened is false.

    def __init__(self):
        self.gc_happened = False

        def it_happened(ignored):
            self.gc_happened = True

        # Create a piece of cyclic trash that triggers it_happened when
        # gc collects it.
        self.wr = weakref.ref(C1055820(666), it_happened)

@with_tp_del
class Uncollectable(object):
    """Create a reference cycle with multiple __del__ methods.

    An object in a reference cycle will never have zero references,
    and so must be garbage collected.  If one or more objects in the
    cycle have __del__ methods, the gc refuses to guess an order,
    and leaves the cycle uncollected."""
    def __init__(self, partner=None):
        if partner is None:
            self.partner = Uncollectable(partner=self)
        else:
            self.partner = partner
    def __tp_del__(self):
        pass


### Tests
###############################################################################

class GCTests(unittest.TestCase):
    def test_list(self):
        l = []
        l.append(l)
        gc.collect()
        del l
        self.assertEqual(gc.collect(), 1)

    def test_dict(self):
        d = {}
        d[1] = d
        gc.collect()
        del d
        self.assertEqual(gc.collect(), 1)

    def test_tuple(self):
        # since tuples are immutable we close the loop with a list
        l = []
        t = (l,)
        l.append(t)
        gc.collect()
        del t
        del l
        self.assertEqual(gc.collect(), 2)

    def test_class(self):
        class A:
            pass
        A.a = A
        gc.collect()
        del A
        self.assertNotEqual(gc.collect(), 0)

    def test_newstyleclass(self):
        class A(object):
            pass
        gc.collect()
        del A
        self.assertNotEqual(gc.collect(), 0)

    def test_instance(self):
        class A:
            pass
        a = A()
        a.a = a
        gc.collect()
        del a
        self.assertNotEqual(gc.collect(), 0)

    def test_newinstance(self):
        class A(object):
            pass
        a = A()
        a.a = a
        gc.collect()
        del a
        self.assertNotEqual(gc.collect(), 0)
        class B(list):
            pass
        class C(B, A):
            pass
        a = C()
        a.a = a
        gc.collect()
        del a
        self.assertNotEqual(gc.collect(), 0)
        del B, C
        self.assertNotEqual(gc.collect(), 0)
        A.a = A()
        del A
        self.assertNotEqual(gc.collect(), 0)
        self.assertEqual(gc.collect(), 0)

    def test_method(self):
        # Tricky: self.__init__ is a bound method, it references the instance.
        class A:
            def __init__(self):
                self.init = self.__init__
        a = A()
        gc.collect()
        del a
        self.assertNotEqual(gc.collect(), 0)

    @cpython_only
    def test_legacy_finalizer(self):
        # A() is uncollectable if it is part of a cycle, make sure it shows up
        # in gc.garbage.
        @with_tp_del
        class A:
            def __tp_del__(self): pass
        class B:
            pass
        a = A()
        a.a = a
        id_a = id(a)
        b = B()
        b.b = b
        gc.collect()
        del a
        del b
        self.assertNotEqual(gc.collect(), 0)
        for obj in gc.garbage:
            if id(obj) == id_a:
                del obj.a
                break
        else:
            self.fail("didn't find obj in garbage (finalizer)")
        gc.garbage.remove(obj)

    @cpython_only
    def test_legacy_finalizer_newclass(self):
        # A() is uncollectable if it is part of a cycle, make sure it shows up
        # in gc.garbage.
        @with_tp_del
        class A(object):
            def __tp_del__(self): pass
        class B(object):
            pass
        a = A()
        a.a = a
        id_a = id(a)
        b = B()
        b.b = b
        gc.collect()
        del a
        del b
        self.assertNotEqual(gc.collect(), 0)
        for obj in gc.garbage:
            if id(obj) == id_a:
                del obj.a
                break
        else:
            self.fail("didn't find obj in garbage (finalizer)")
        gc.garbage.remove(obj)

    def test_function(self):
        # Tricky: f -> d -> f, code should call d.clear() after the exec to
        # break the cycle.
        d = {}
        exec("def f(): pass\n", d)
        gc.collect()
        del d
        # In the free-threaded build, the count returned by `gc.collect()`
        # is 3 because it includes f's code object.
        self.assertIn(gc.collect(), (2, 3))

    def test_function_tp_clear_leaves_consistent_state(self):
        # https://github.com/python/cpython/issues/91636
        code = """if 1:

        import gc
        import weakref

        class LateFin:
            __slots__ = ('ref',)

            def __del__(self):

                # 8. Now `latefin`'s finalizer is called. Here we
                #    obtain a reference to `func`, which is currently
                #    undergoing `tp_clear`.
                global func
                func = self.ref()

        class Cyclic(tuple):
            __slots__ = ()

            # 4. The finalizers of all garbage objects are called. In
            #    this case this is only us as `func` doesn't have a
            #    finalizer.
            def __del__(self):

                # 5. Create a weakref to `func` now. In previous
                #    versions of Python, this would avoid having it
                #    cleared by the garbage collector before calling
                #    the finalizers.  Now, weakrefs get cleared after
                #    calling finalizers.
                self[1].ref = weakref.ref(self[0])

                # 6. Drop the global reference to `latefin`. The only
                #    remaining reference is the one we have.
                global latefin
                del latefin

            # 7. Now `func` is `tp_clear`-ed. This drops the last
            #    reference to `Cyclic`, which gets `tp_dealloc`-ed.
            #    This drops the last reference to `latefin`.

        latefin = LateFin()
        def func():
            pass
        cyc = tuple.__new__(Cyclic, (func, latefin))

        # 1. Create a reference cycle of `cyc` and `func`.
        func.__module__ = cyc

        # 2. Make the cycle unreachable, but keep the global reference
        #    to `latefin` so that it isn't detected as garbage. This
        #    way its finalizer will not be called immediately.
        del func, cyc

        # 3. Invoke garbage collection,
        #    which will find `cyc` and `func` as garbage.
        gc.collect()

        # 9. Previously, this would crash because the weakref
        #    created in the finalizer revealed the function after
        #    `tp_clear` was called and `func_qualname`
        #    had been NULL-ed out by func_clear().  Now, we clear
        #    weakrefs to unreachable objects before calling `tp_clear`
        #    but after calling finalizers.
        print(f"{func=}")
        """
        rc, stdout, stderr = assert_python_ok("-c", code)
        self.assertEqual(rc, 0)
        # The `func` global is None because the weakref was cleared.
        self.assertRegex(stdout, rb"""\A\s*func=None""")
        self.assertFalse(stderr)

    def test_datetime_weakref_cycle(self):
        # https://github.com/python/cpython/issues/132413
        # If the weakref used by the datetime extension gets cleared by the GC (due to being
        # in an unreachable cycle) then datetime functions would crash (get_module_state()
        # was returning a NULL pointer).  This bug is fixed by clearing weakrefs without
        # callbacks *after* running finalizers.
        code = """if 1:
        import _datetime
        class C:
            def __del__(self):
                print('__del__ called')
                _datetime.timedelta(days=1)  # crash?

        l = [C()]
        l.append(l)
        """
        rc, stdout, stderr = assert_python_ok("-c", code)
        self.assertEqual(rc, 0)
        self.assertEqual(stdout.strip(), b'__del__ called')

    def test_del(self):
        # __del__ methods can trigger collection, make this to happen
        thresholds = gc.get_threshold()
        gc.enable()
        gc.set_threshold(1)

        class A:
            def __del__(self):
                dir(self)
        a = A()
        del a

        gc.disable()
        gc.set_threshold(*thresholds)

    def test_del_newclass(self):
        # __del__ methods can trigger collection, make this to happen
        thresholds = gc.get_threshold()
        gc.enable()
        gc.set_threshold(1)

        class A(object):
            def __del__(self):
                dir(self)
        a = A()
        del a

        gc.disable()
        gc.set_threshold(*thresholds)

    def test_trashcan(self):
        class Ouch:
            n = 0
            def __del__(self):
                Ouch.n = Ouch.n + 1
                if Ouch.n % 17 == 0:
                    gc.collect()

        # "trashcan" is a hack to prevent stack overflow when deallocating
        # very deeply nested tuples etc.  It works in part by abusing the
        # type pointer and refcount fields, and that can yield horrible
        # problems when gc tries to traverse the structures.
        # If this test fails (as it does in 2.0, 2.1 and 2.2), it will
        # most likely die via segfault.

        # Note:  In 2.3 the possibility for compiling without cyclic gc was
        # removed, and that in turn allows the trashcan mechanism to work
        # via much simpler means (e.g., it never abuses the type pointer or
        # refcount fields anymore).  Since it's much less likely to cause a
        # problem now, the various constants in this expensive (we force a lot
        # of full collections) test are cut back from the 2.2 version.
        gc.enable()
        N = 150
        for count in range(2):
            t = []
            for i in range(N):
                t = [t, Ouch()]
            u = []
            for i in range(N):
                u = [u, Ouch()]
            v = {}
            for i in range(N):
                v = {1: v, 2: Ouch()}
        gc.disable()

    @threading_helper.requires_working_threading()
    def test_trashcan_threads(self):
        # Issue #13992: trashcan mechanism should be thread-safe
        NESTING = 60
        N_THREADS = 2

        def sleeper_gen():
            """A generator that releases the GIL when closed or dealloc'ed."""
            try:
                yield
            finally:
                time.sleep(0.000001)

        class C(list):
            # Appending to a list is atomic, which avoids the use of a lock.
            inits = []
            dels = []
            def __init__(self, alist):
                self[:] = alist
                C.inits.append(None)
            def __del__(self):
                # This __del__ is called by subtype_dealloc().
                C.dels.append(None)
                # `g` will release the GIL when garbage-collected.  This
                # helps assert subtype_dealloc's behaviour when threads
                # switch in the middle of it.
                g = sleeper_gen()
                next(g)
                # Now that __del__ is finished, subtype_dealloc will proceed
                # to call list_dealloc, which also uses the trashcan mechanism.

        def make_nested():
            """Create a sufficiently nested container object so that the
            trashcan mechanism is invoked when deallocating it."""
            x = C([])
            for i in range(NESTING):
                x = [C([x])]
            del x

        def run_thread():
            """Exercise make_nested() in a loop."""
            while not exit:
                make_nested()

        # The collector finalizes these objects only when it runs.
        gc.enable()
        self.addCleanup(gc.disable)
        old_switchinterval = sys.getswitchinterval()
        support.setswitchinterval(1e-5)
        try:
            exit = []
            threads = []
            for i in range(N_THREADS):
                t = threading.Thread(target=run_thread)
                threads.append(t)
            with threading_helper.start_threads(threads, lambda: exit.append(1)):
                time.sleep(1.0)
        finally:
            sys.setswitchinterval(old_switchinterval)
        gc.collect()
        self.assertEqual(len(C.inits), len(C.dels))

    def test_boom(self):
        class Boom:
            def __getattr__(self, someattribute):
                del self.attr
                raise AttributeError

        a = Boom()
        b = Boom()
        a.attr = b
        b.attr = a

        gc.collect()
        garbagelen = len(gc.garbage)
        del a, b
        # a<->b are in a trash cycle now.  Collection will invoke
        # Boom.__getattr__ (to see whether a and b have __del__ methods), and
        # __getattr__ deletes the internal "attr" attributes as a side effect.
        # That causes the trash cycle to get reclaimed via refcounts falling to
        # 0, thus mutating the trash graph as a side effect of merely asking
        # whether __del__ exists.  This used to (before 2.3b1) crash Python.
        # Now __getattr__ isn't called.
        self.assertEqual(gc.collect(), 2)
        self.assertEqual(len(gc.garbage), garbagelen)

    def test_boom2(self):
        class Boom2:
            def __init__(self):
                self.x = 0

            def __getattr__(self, someattribute):
                self.x += 1
                if self.x > 1:
                    del self.attr
                raise AttributeError

        a = Boom2()
        b = Boom2()
        a.attr = b
        b.attr = a

        gc.collect()
        garbagelen = len(gc.garbage)
        del a, b
        # Much like test_boom(), except that __getattr__ doesn't break the
        # cycle until the second time gc checks for __del__.  As of 2.3b1,
        # there isn't a second time, so this simply cleans up the trash cycle.
        # We expect a, b, a.__dict__ and b.__dict__ (4 objects) to get
        # reclaimed this way.
        self.assertEqual(gc.collect(), 2)
        self.assertEqual(len(gc.garbage), garbagelen)

    def test_get_referents(self):
        # Integers beyond int32 are objects; smaller ones and floats are
        # inline values, which are not referents.
        big = 2**40
        alist = [big + 1, big + 3, big + 5]
        got = gc.get_referents(alist)
        got.sort()
        self.assertEqual(got, alist)

        atuple = tuple(alist)
        got = gc.get_referents(atuple)
        got.sort()
        self.assertEqual(got, alist)

        adict = {big + 1: big + 3, big + 5: big + 7}
        expected = [big + 1, big + 3, big + 5, big + 7]
        got = gc.get_referents(adict)
        got.sort()
        self.assertEqual(got, expected)

        got = gc.get_referents([big + 1, big + 2], {big + 3: big + 4},
                               (big, big, big))
        got.sort()
        self.assertEqual(got, [big, big] + [big + i for i in range(5)])

        self.assertEqual(gc.get_referents([1, 2.5], (3, 4.5)), [])
        self.assertEqual(gc.get_referents(1, 'a', 4j), [])

    def test_is_tracked(self):
        # Atomic built-in types are not tracked, user-defined objects and
        # mutable containers are.
        # NOTE: types with special optimizations (e.g. tuple) have tests
        # in their own test files instead.
        self.assertFalse(gc.is_tracked(None))
        self.assertFalse(gc.is_tracked(1))
        self.assertFalse(gc.is_tracked(1.0))
        self.assertFalse(gc.is_tracked(1.0 + 5.0j))
        self.assertFalse(gc.is_tracked(True))
        self.assertFalse(gc.is_tracked(False))
        self.assertFalse(gc.is_tracked(b"a"))
        self.assertFalse(gc.is_tracked("a"))
        self.assertFalse(gc.is_tracked(bytearray(b"a")))
        self.assertFalse(gc.is_tracked(type))
        self.assertFalse(gc.is_tracked(int))
        self.assertFalse(gc.is_tracked(object))
        self.assertFalse(gc.is_tracked(object()))

        class UserClass:
            pass

        class UserInt(int):
            pass

        # Base class is object; no extra fields.
        class UserClassSlots:
            __slots__ = ()

        # Base class is fixed size larger than object; no extra fields.
        class UserFloatSlots(float):
            __slots__ = ()

        # Base class is variable size; no extra fields.
        class UserIntSlots(int):
            __slots__ = ()

        self.assertTrue(gc.is_tracked(gc))
        self.assertTrue(gc.is_tracked(UserClass))
        self.assertTrue(gc.is_tracked(UserClass()))
        self.assertTrue(gc.is_tracked(UserInt()))
        self.assertTrue(gc.is_tracked([]))
        self.assertTrue(gc.is_tracked(set()))
        self.assertTrue(gc.is_tracked(UserClassSlots()))
        self.assertTrue(gc.is_tracked(UserFloatSlots()))
        self.assertTrue(gc.is_tracked(UserIntSlots()))

    def test_is_finalized(self):
        # Objects not tracked by the always gc return false
        self.assertFalse(gc.is_finalized(3))

        storage = []
        class Lazarus:
            def __del__(self):
                storage.append(self)

        lazarus = Lazarus()
        self.assertFalse(gc.is_finalized(lazarus))

        del lazarus
        gc.collect()

        lazarus = storage.pop()
        self.assertTrue(gc.is_finalized(lazarus))

    def test_bug1055820b(self):
        # Corresponds to temp2b.py in the bug report.

        ouch = []
        def callback(ignored):
            ouch[:] = [wr() for wr in WRs]

        Cs = [C1055820(i) for i in range(2)]
        WRs = [weakref.ref(c, callback) for c in Cs]
        c = None

        gc.collect()
        self.assertEqual(len(ouch), 0)
        # Make the two instances trash, and collect again.  The bug was that
        # the callback materialized a strong reference to an instance, but gc
        # cleared the instance's dict anyway.
        Cs = None
        gc.collect()
        self.assertEqual(len(ouch), 2)  # else the callbacks didn't run
        for x in ouch:
            # The weakref should be cleared before executing the callback.
            self.assertIsNone(x)

    def test_bug21435(self):
        # This is a poor test - its only virtue is that it happened to
        # segfault on Tim's Windows box before the patch for 21435 was
        # applied.  That's a nasty bug relying on specific pieces of cyclic
        # trash appearing in exactly the right order in finalize_garbage()'s
        # input list.
        # But there's no reliable way to force that order from Python code,
        # so over time chances are good this test won't really be testing much
        # of anything anymore.  Still, if it blows up, there's _some_
        # problem ;-)
        gc.collect()

        class A:
            pass

        class B:
            def __init__(self, x):
                self.x = x

            def __del__(self):
                self.attr = None

        def do_work():
            a = A()
            b = B(A())

            a.attr = b
            b.attr = a

        do_work()
        gc.collect() # this blows up (bad C pointer) when it fails

    @cpython_only
    @requires_subprocess()
    @unittest.skipIf(_testcapi is None, "requires _testcapi")
    def test_garbage_at_shutdown(self):
        import subprocess
        code = """if 1:
            import gc
            import _testcapi
            @_testcapi.with_tp_del
            class X:
                def __init__(self, name):
                    self.name = name
                def __repr__(self):
                    return "<X %%r>" %% self.name
                def __tp_del__(self):
                    pass

            x = X('first')
            x.x = x
            x.y = X('second')
            del x
            gc.set_debug(%s)
        """
        def run_command(code):
            p = subprocess.Popen([sys.executable, "-Wd", "-c", code],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE)
            stdout, stderr = p.communicate()
            p.stdout.close()
            p.stderr.close()
            self.assertEqual(p.returncode, 0)
            self.assertEqual(stdout, b"")
            return stderr

        stderr = run_command(code % "0")
        self.assertIn(b"ResourceWarning: gc: 2 uncollectable objects at "
                      b"shutdown; use", stderr)
        self.assertNotIn(b"<X 'first'>", stderr)
        one_line_re = b"gc: uncollectable <X 0x[0-9A-Fa-f]+>"
        expected_re = one_line_re + b"\r?\n" + one_line_re
        self.assertNotRegex(stderr, expected_re)
        # With DEBUG_UNCOLLECTABLE, the garbage list gets printed
        stderr = run_command(code % "gc.DEBUG_UNCOLLECTABLE")
        self.assertIn(b"ResourceWarning: gc: 2 uncollectable objects at "
                      b"shutdown", stderr)
        self.assertTrue(
            (b"[<X 'first'>, <X 'second'>]" in stderr) or
            (b"[<X 'second'>, <X 'first'>]" in stderr), stderr)
        # we expect two lines with uncollectable objects
        self.assertRegex(stderr, expected_re)
        # With DEBUG_SAVEALL, no additional message should get printed
        # (because gc.garbage also contains normally reclaimable cyclic
        # references, and its elements get printed at runtime anyway).
        stderr = run_command(code % "gc.DEBUG_SAVEALL")
        self.assertNotIn(b"uncollectable objects at shutdown", stderr)

    def test_gc_main_module_at_shutdown(self):
        # Create a reference cycle through the __main__ module and check
        # it gets collected at interpreter shutdown.
        code = """if 1:
            class C:
                def __del__(self):
                    print('__del__ called')
            l = [C()]
            l.append(l)
            """
        rc, out, err = assert_python_ok('-c', code)
        self.assertEqual(out.strip(), b'__del__ called')

    def test_gc_ordinary_module_at_shutdown(self):
        # Same as above, but with a non-__main__ module.
        with temp_dir() as script_dir:
            module = """if 1:
                class C:
                    def __del__(self):
                        print('__del__ called')
                l = [C()]
                l.append(l)
                """
            code = """if 1:
                import sys
                sys.path.insert(0, %r)
                import gctest
                """ % (script_dir,)
            make_script(script_dir, 'gctest', module)
            rc, out, err = assert_python_ok('-c', code)
            self.assertEqual(out.strip(), b'__del__ called')

    def test_global_del_SystemExit(self):
        code = """if 1:
            class ClassWithDel:
                def __del__(self):
                    print('__del__ called')
            a = ClassWithDel()
            a.link = a
            raise SystemExit(0)"""
        self.addCleanup(unlink, TESTFN)
        with open(TESTFN, 'w', encoding="utf-8") as script:
            script.write(code)
        rc, out, err = assert_python_ok(TESTFN)
        self.assertEqual(out.strip(), b'__del__ called')

    def test_get_objects(self):
        gc.collect()
        l = []
        l.append(l)
        self.assertTrue(
                any(l is element for element in gc.get_objects())
        )

    def test_get_objects_arguments(self):
        gc.collect()
        self.assertEqual(len(gc.get_objects()),
                         len(gc.get_objects(generation=None)))

        self.assertRaises(ValueError, gc.get_objects, 1000)
        self.assertRaises(ValueError, gc.get_objects, -1000)
        self.assertRaises(TypeError, gc.get_objects, "1")
        self.assertRaises(TypeError, gc.get_objects, 1.234)

    def test_resurrection_only_happens_once_per_object(self):
        class A:  # simple self-loop
            def __init__(self):
                self.me = self

        class Lazarus(A):
            resurrected = 0
            resurrected_instances = []

            def __del__(self):
                Lazarus.resurrected += 1
                Lazarus.resurrected_instances.append(self)

        gc.collect()
        gc.disable()

        # We start with 0 resurrections
        laz = Lazarus()
        self.assertEqual(Lazarus.resurrected, 0)

        # Deleting the instance and triggering a collection
        # resurrects the object
        del laz
        gc.collect()
        self.assertEqual(Lazarus.resurrected, 1)
        self.assertEqual(len(Lazarus.resurrected_instances), 1)

        # Clearing the references and forcing a collection
        # should not resurrect the object again.
        Lazarus.resurrected_instances.clear()
        self.assertEqual(Lazarus.resurrected, 1)
        gc.collect()
        self.assertEqual(Lazarus.resurrected, 1)

        gc.enable()

    def test_resurrection_is_transitive(self):
        class Cargo:
            def __init__(self):
                self.me = self

        class Lazarus:
            resurrected_instances = []

            def __del__(self):
                Lazarus.resurrected_instances.append(self)

        gc.collect()
        gc.disable()

        laz = Lazarus()
        cargo = Cargo()
        cargo_id = id(cargo)

        # Create a cycle between cargo and laz
        laz.cargo = cargo
        cargo.laz = laz

        # Drop the references, force a collection and check that
        # everything was resurrected.
        del laz, cargo
        gc.collect()
        self.assertEqual(len(Lazarus.resurrected_instances), 1)
        instance = Lazarus.resurrected_instances.pop()
        self.assertHasAttr(instance, "cargo")
        self.assertEqual(id(instance.cargo), cargo_id)

        gc.collect()
        gc.enable()

    @unittest.skipIf(ContainerNoGC is None,
                     'requires ContainerNoGC extension type')
    def test_trash_weakref_clear(self):
        # Test that trash weakrefs are properly cleared (bpo-38006).
        #
        # Structure we are creating:
        #
        #   Z <- Y <- A--+--> WZ -> C
        #             ^  |
        #             +--+
        # where:
        #   WZ is a weakref to Z with callback C
        #   Y doesn't implement tp_traverse
        #   A contains a reference to itself, Y and WZ
        #
        # A, Y, Z, WZ are all trash.  The GC doesn't know that Z is trash
        # because Y does not implement tp_traverse.  To show the bug, WZ needs
        # to live long enough so that Z is deallocated before it.  Then, if
        # gcmodule is buggy, when Z is being deallocated, C will run.
        #
        # To ensure WZ lives long enough, we put it in a second reference
        # cycle.  That trick only works due to the ordering of the GC prev/next
        # linked lists.  So, this test is a bit fragile.
        #
        # The bug reported in bpo-38006 is caused because the GC did not
        # clear WZ before starting the process of calling tp_clear on the
        # trash.  Normally, handle_weakrefs() would find the weakref via Z and
        # clear it.  However, since the GC cannot find Z, WR is not cleared and
        # it can execute during delete_garbage().  That can lead to disaster
        # since the callback might tinker with objects that have already had
        # tp_clear called on them (leaving them in possibly invalid states).

        callback = unittest.mock.Mock()

        class A:
            __slots__ = ['a', 'y', 'wz']

        class Z:
            pass

        # setup required object graph, as described above
        a = A()
        a.a = a
        a.y = ContainerNoGC(Z())
        a.wz = weakref.ref(a.y.value, callback)
        # create second cycle to keep WZ alive longer
        wr_cycle = [a.wz]
        wr_cycle.append(wr_cycle)
        # ensure trash unrelated to this test is gone
        gc.collect()
        gc.disable()
        # release references and create trash
        del a, wr_cycle
        gc.collect()
        # if called, it means there is a bug in the GC.  The weakref should be
        # cleared before Z dies.
        callback.assert_not_called()
        gc.enable()

    @cpython_only
    def test_get_objects_during_gc(self):
        # gh-125859: Calling gc.get_objects() or gc.get_referrers() during a
        # collection should not crash.
        collected = False

        class GetObjectsOnDel:
            def __del__(self):
                nonlocal collected
                collected = True
                # The finalizer runs after the collection, on a live object
                # that both functions see; they must not crash.
                gc.get_objects()
                gc.get_referrers(self)

        obj = GetObjectsOnDel()
        obj.cycle = obj
        del obj

        gc.collect()
        self.assertTrue(collected)

    def test_traverse_frozen_objects(self):
        # See GH-126312: Objects that were not frozen could traverse over
        # a frozen object on the free-threaded build, which would cause
        # a negative reference count.
        x = [1, 2, 3]
        gc.freeze()
        y = [x]
        y.append(y)
        del y
        gc.collect()
        gc.unfreeze()

    def test_deferred_refcount_frozen(self):
        # Also from GH-126312: objects that use deferred reference counting
        # weren't ignored if they were frozen. Unfortunately, it's pretty
        # difficult to come up with a case that triggers this.
        #
        # Calling gc.collect() while the garbage collector is frozen doesn't
        # trigger this normally, but it *does* if it's inside unittest for whatever
        # reason. We can't call unittest from inside a test, so it has to be
        # in a subprocess.
        source = textwrap.dedent("""
        import gc
        import unittest


        class Test(unittest.TestCase):
            def test_something(self):
                gc.freeze()
                gc.collect()
                gc.unfreeze()


        if __name__ == "__main__":
            unittest.main()
        """)
        assert_python_ok("-c", source)

    def test_do_not_cleanup_type_subclasses_before_finalization(self):
        #  See https://github.com/python/cpython/issues/135552
        # If we cleanup weakrefs for tp_subclasses before calling
        # the finalizer (__del__) then the line `fail = BaseNode.next.next`
        # should fail because we are trying to access a subclass
        # attribute. But subclass type cache was not properly invalidated.
        code = """
            class BaseNode:
                def __del__(self):
                    BaseNode.next = BaseNode.next.next
                    fail = BaseNode.next.next

            class Node(BaseNode):
                pass

            BaseNode.next = Node()
            BaseNode.next.next = Node()
        """
        # this test checks garbage collection while interp
        # finalization
        assert_python_ok("-c", textwrap.dedent(code))

        code_inside_function = textwrap.dedent(F"""
            def test():
                {textwrap.indent(code, '    ')}

            test()
        """)
        # this test checks regular garbage collection
        assert_python_ok("-c", code_inside_function)


    @unittest.skipUnless(Py_GIL_DISABLED, "requires free-threaded GC")
    @unittest.skipIf(_testinternalcapi is None, "requires _testinternalcapi")
    def test_tuple_untrack_counts(self):
        # This ensures that the free-threaded GC is counting untracked tuples
        # in the "long_lived_total" count.  This is required to avoid
        # performance issues from running the GC too frequently.  See
        # GH-142531 as an example.
        gc.collect()
        count = _testinternalcapi.get_long_lived_total()
        n = 20_000
        tuples = [(x,) for x in range(n)]
        gc.collect()
        new_count = _testinternalcapi.get_long_lived_total()
        self.assertFalse(gc.is_tracked(tuples[0]))
        # Use n // 2 just in case some other objects were collected.
        self.assertTrue(new_count - count > (n // 2))


class GCCallbackTests(unittest.TestCase):
    def setUp(self):
        # Save gc state and disable it.
        self.enabled = gc.isenabled()
        gc.disable()
        self.debug = gc.get_debug()
        gc.set_debug(0)
        gc.callbacks.append(self.cb1)
        gc.callbacks.append(self.cb2)
        self.othergarbage = []

    def tearDown(self):
        # Restore gc state
        del self.visit
        gc.callbacks.remove(self.cb1)
        gc.callbacks.remove(self.cb2)
        gc.set_debug(self.debug)
        if self.enabled:
            gc.enable()
        # destroy any uncollectables
        gc.collect()
        for obj in gc.garbage:
            if isinstance(obj, Uncollectable):
                obj.partner = None
        del gc.garbage[:]
        del self.othergarbage
        gc.collect()

    def preclean(self):
        # Remove all fluff from the system.  Invoke this function
        # manually rather than through self.setUp() for maximum
        # safety.
        self.visit = []
        gc.collect()
        garbage, gc.garbage[:] = gc.garbage[:], []
        self.othergarbage.append(garbage)
        self.visit = []

    def cb1(self, phase, info):
        self.visit.append((1, phase, dict(info)))

    def cb2(self, phase, info):
        self.visit.append((2, phase, dict(info)))
        if phase == "stop" and hasattr(self, "cleanup"):
            # Clean Uncollectable from garbage
            uc = [e for e in gc.garbage if isinstance(e, Uncollectable)]
            gc.garbage[:] = [e for e in gc.garbage
                             if not isinstance(e, Uncollectable)]
            for e in uc:
                e.partner = None

    def test_collect(self):
        self.preclean()
        gc.collect()
        # Algorithmically verify the contents of self.visit
        # because it is long and tortuous.

        # Count the number of visits to each callback
        n = [v[0] for v in self.visit]
        n1 = [i for i in n if i == 1]
        n2 = [i for i in n if i == 2]
        self.assertEqual(n1, [1]*2)
        self.assertEqual(n2, [2]*2)

        # Count that we got the right number of start and stop callbacks.
        n = [v[1] for v in self.visit]
        n1 = [i for i in n if i == "start"]
        n2 = [i for i in n if i == "stop"]
        self.assertEqual(n1, ["start"]*2)
        self.assertEqual(n2, ["stop"]*2)

        # Check that we got the right info dict for all callbacks
        for v in self.visit:
            info = v[2]
            self.assertIn("generation", info)
            self.assertIn("collected", info)
            self.assertIn("uncollectable", info)
            self.assertIn("candidates", info)
            self.assertIn("duration", info)

    def test_collect_generation(self):
        self.preclean()
        gc.collect(2)
        for v in self.visit:
            info = v[2]
            self.assertEqual(info["generation"], 2)


class GCTogglingTests(unittest.TestCase):
    def setUp(self):
        gc.enable()

    def tearDown(self):
        gc.disable()

    @gc_threshold(1000, 0, 0)
    @unittest.skipIf(Py_GIL_DISABLED, "requires GC generations or increments")
    def test_bug1055820d(self):
        # Corresponds to temp2d.py in the bug report.  This is very much like
        # test_bug1055820c, but uses a __del__ method instead of a weakref
        # callback to sneak in a resurrection of cyclic trash.

        ouch = []
        class D(C1055820):
            def __del__(self):
                ouch[:] = [c2wr()]

        d0 = D(0)
        # Move all the above into generation 2.
        gc.collect()

        c1 = C1055820(1)
        c1.keep_d0_alive = d0
        del d0.loop # now only c1 keeps d0 alive

        c2 = C1055820(2)
        c2wr = weakref.ref(c2) # no callback!

        d0 = c1 = c2 = None

        # What we've set up:  d0, c1, and c2 are all trash now.  d0 is in
        # generation 2.  The only thing keeping it alive is that c1 points to
        # it.  c1 and c2 are in generation 0, and are in self-loops.  There's
        # a global weakref to c2 (c2wr), but that weakref has no callback.
        # There are no other weakrefs.
        #
        #               d0 has a __del__ method that references c2wr
        #               ^
        #               |
        #               |     Generation 2 above dots
        #. . . . . . . .|. . . . . . . . . . . . . . . . . . . . . . . .
        #               |     Generation 0 below dots
        #               |
        #               |
        #            ^->c1   ^->c2 has a wr but no callback
        #            |  |    |  |
        #            <--v    <--v
        #
        # So this is the nightmare:  when generation 0 gets collected, we see
        # that c2 has a callback-free weakref, and c1 doesn't even have a
        # weakref.  Collecting generation 0 doesn't see d0 at all.  gc clears
        # c1 and c2.  Clearing c1 has the side effect of dropping the refcount
        # on d0 to 0, so d0 goes away (despite that it's in an older
        # generation) and d0's __del__ triggers.  That in turn materializes
        # a reference to c2 via c2wr(), but c2 gets cleared anyway by gc.

        # We want to let gc happen "naturally", to preserve the distinction
        # between generations.
        detector = GC_Detector()
        junk = []
        i = 0
        # There is one generation, so just trigger a GC manually.
        gc.collect()
        while not detector.gc_happened:
            i += 1
            if i > 10000:
                self.fail("gc didn't happen after 10000 iterations")
            self.assertEqual(len(ouch), 0)
            junk.append([])  # this will eventually trigger gc

        self.assertEqual(len(ouch), 1)  # else __del__ wasn't invoked
        for x in ouch:
            # If __del__ resurrected c2, the instance would be damaged, with an
            # empty __dict__.
            self.assertEqual(x, None)

    def test_get_count_nonnegative(self):
        xs = [[] for _ in range(1500)]
        gc.collect()
        del xs[:1000]
        self.assertGreaterEqual(gc.get_count()[0], 0)

    # Ensure that setting *threshold0* to zero disables collection.
    @gc_threshold(0)
    def test_threshold_zero(self):
        junk = []
        i = 0
        detector = GC_Detector()
        while not detector.gc_happened:
            i += 1
            if i > 50000:
                break
            junk.append([])  # this may eventually trigger gc (if it is enabled)

        self.assertEqual(i, 50001)


class TracingCollectorTests(unittest.TestCase):
    """The behaviour that the tracing collector adds."""

    # The conservative scan of C stacks can keep one attempt's garbage alive;
    # the tests that need garbage freed try again with new objects.
    ATTEMPTS = 5

    def setUp(self):
        gc.collect()
        gc.enable()
        self.addCleanup(gc.disable)

    def test_threshold_is_bytes(self):
        self.assertEqual(gc.get_threshold()[0], 32 * 1024 * 1024)
        gc.collect()
        before = gc.get_count()[0]
        junk = [bytes(1000) for _ in range(1000)]
        self.assertGreaterEqual(gc.get_count()[0] - before, 1000 * 1000)
        self.assertEqual(gc.get_count()[1:], (0, 0))
        gc.collect()
        self.assertLess(gc.get_count()[0], 1000 * 1000)
        del junk

    def collections_while_allocating(self, nbytes):
        starts = []
        def callback(phase, info):
            if phase == "start":
                starts.append(info["generation"])
        gc.callbacks.append(callback)
        try:
            for _ in range(nbytes // 1000):
                bytes(1000)
        finally:
            gc.callbacks.remove(callback)
        return starts

    @gc_threshold(1024 * 1024)
    def test_allocation_triggers_collection(self):
        # The garbage keeps the live heap below the allocated bytes.
        self.assertGreater(len(self.collections_while_allocating(20 << 20)),
                           0)

    @gc_threshold(1024 * 1024)
    def test_disable_stops_automatic_collection(self):
        gc.disable()
        self.assertEqual(self.collections_while_allocating(20 << 20), [])

    @gc_threshold(0)
    def test_threshold_zero(self):
        self.assertEqual(self.collections_while_allocating(20 << 20), [])

    def test_get_stats(self):
        stats = gc.get_stats()[2]
        for key in ("bytes_freed", "live_bytes", "max_pause"):
            self.assertIn(key, stats)
        freed = stats["bytes_freed"]
        junk = [bytes(1000) for _ in range(1000)]
        del junk
        gc.collect()
        stats = gc.get_stats()[2]
        self.assertGreater(stats["bytes_freed"], freed)
        self.assertGreater(stats["live_bytes"], 0)
        self.assertGreater(stats["max_pause"], 0)

    def test_collect_frees_garbage_without_cycles(self):
        class A:
            pass
        for _ in range(self.ATTEMPTS):
            a = A()
            ref = weakref.ref(a)
            del a
            gc.collect()
            if ref() is None:
                break
        self.assertIsNone(ref())

    def test_eval_loop_frame_is_not_a_root(self):
        # The collector does not scan the eval loop's own C frames, whose
        # spill slots hold stale copies of values the frame no longer has,
        # so a dead local is freed at the first collection, every time.
        class A:
            pass
        def dead_local():
            a = A()
            a.self = a
            ref = weakref.ref(a)
            del a
            gc.collect()
            return ref() is None
        def dead_item():
            items = [A()]
            ref = weakref.ref(items[0])
            del items
            gc.collect()
            return ref() is None
        for func in dead_local, dead_item:
            with self.subTest(func=func.__name__):
                self.assertEqual(sum(func() for _ in range(20)), 20)

    def test_sweep_frees_exported_memoryview(self):
        # A struct iterator holds an export of its memoryview, and the sweep
        # may deallocate the view first.
        import struct
        for _ in range(100):
            struct.iter_unpack('b', memoryview(bytes(8)))
        gc.collect()

    def test_finalizer_order(self):
        # A referrer is finalized before what it refers to, as reference
        # counting would do: a buffered file flushes before its raw file
        # closes.
        order = []
        class F:
            def __init__(self, name, other=None):
                self.name = name
                self.other = other
            def __del__(self):
                order.append(self.name)
        for _ in range(self.ATTEMPTS):
            order.clear()
            F("outer", F("middle", F("inner")))
            gc.collect()
            if len(order) == 3:
                break
        self.assertEqual(order, ["outer", "middle", "inner"])

    def test_weakref_callback_runs_after_collection(self):
        class A:
            pass
        called = []
        for _ in range(self.ATTEMPTS):
            a = A()
            ref = weakref.ref(a, called.append)
            del a
            gc.collect()
            if called:
                break
        self.assertEqual(called, [ref])
        self.assertIsNone(ref())

    @unittest.skipIf(_testcapi is None, "requires _testcapi")
    def test_dealloc_reads_referents(self):
        # A DeallocReader's dealloc reads its referent's state and aborts if
        # the referent's dealloc ran first.  The referents are allocated
        # first, so without Py_TPFLAGS_DEALLOC_READS_REFERENTS the sweep,
        # which goes in heap order, would reach them first.
        Py_TPFLAGS_DEALLOC_READS_REFERENTS = 1 << 21
        class Sub(_testcapi.DeallocReader):
            pass
        self.assertTrue(Sub.__flags__ & Py_TPFLAGS_DEALLOC_READS_REFERENTS)
        referents = [_testcapi.StatefulReferent() for _ in range(1000)]
        readers = [(Sub if i % 2 else _testcapi.DeallocReader)(r)
                   for i, r in enumerate(referents)]
        del referents, readers
        gc.collect()

    @unittest.skipIf(_testcapi is None, "requires _testcapi")
    def test_dealloc_reads_flagged_referents(self):
        # Each reader in a chain reads the reader it refers to, which was
        # allocated before it: the sweep must go referrer first among the
        # flagged objects, not in heap order.
        class Sub(_testcapi.DeallocReader):
            pass
        chains = []
        for i in range(300):
            r = _testcapi.StatefulReferent()
            for j in range(5):
                r = (Sub if (i + j) % 2 else _testcapi.DeallocReader)(r)
            chains.append(r)
        del chains, r
        gc.collect()

    @unittest.skipIf(_testcapi is None, "requires _testcapi")
    def test_dealloc_reads_referents_in_a_cycle(self):
        # Readers that refer to each other through their attributes, which
        # their deallocs do not read: the sweep breaks the cycle.
        class Sub(_testcapi.DeallocReader):
            pass
        for n in (1, 2, 3, 50):
            readers = [Sub(_testcapi.StatefulReferent()) for _ in range(n)]
            for i, r in enumerate(readers):
                r.next = readers[(i + 1) % n]
            del readers, r
            gc.collect()

    @unittest.skipIf(_testcapi is None, "requires _testcapi")
    def test_add_root(self):
        class A:
            pass
        a = A()
        ref = weakref.ref(a)
        _testcapi.gc_hold_in_pymem(a, True)
        del a
        gc.collect()
        self.assertIsNotNone(ref())
        a = _testcapi.gc_release_pymem()
        self.assertIs(a, ref())

    @unittest.skipIf(_testcapi is None, "requires _testcapi")
    def test_capsule_destructor_runs_python_code(self):
        # The collector calls a capsule's destructor before the sweep, which
        # may not run code (a debug build aborts if it does).
        called = []
        def callback():
            called.append([1])
        for _ in range(self.ATTEMPTS):
            _testcapi.capsule_calling(callback)
            gc.collect()
            if called:
                break
        self.assertEqual(called, [[1]])

    @unittest.skipIf(_testcapi is None, "requires _testcapi")
    def test_unrooted_pymem_reference_is_freed(self):
        # The pattern that PyGC_AddRoot() fixes.
        class A:
            pass
        for _ in range(self.ATTEMPTS):
            a = A()
            ref = weakref.ref(a)
            _testcapi.gc_hold_in_pymem(a, False)
            del a
            gc.collect()
            _testcapi.gc_release_pymem()
            if ref() is None:
                break
        self.assertIsNone(ref())

    @threading_helper.requires_working_threading()
    def test_daemon_thread_keeps_its_objects_at_shutdown(self):
        # Finalization unlinks the state of a daemon thread blocked in a
        # read, which holds the file's lock.  Its frames still hold the file:
        # closing it at a shutdown collection would wait for the lock.
        code = textwrap.dedent("""
            import os, threading
            r, w = os.pipe()
            f = os.fdopen(r, 'rb')
            started = threading.Event()
            def reader():
                started.set()
                f.read(10)
            threading.Thread(target=reader, daemon=True).start()
            started.wait()
            del f
        """)
        assert_python_ok("-c", code)

    def test_release_buffer_runs_python_code(self):
        # A dead view of an exporter that defines __release_buffer__ is
        # released before the sweep, which may not run code (a debug build
        # aborts if it does).
        released = []
        class B(bytearray):
            def __buffer__(self, flags):
                return super().__buffer__(flags)
            def __release_buffer__(self, view):
                released.append(1)
                super().__release_buffer__(view)
        b = B(b"hello")
        for _ in range(self.ATTEMPTS):
            memoryview(b)
            gc.collect()
            if released:
                break
        self.assertTrue(released)

    @unittest.skipIf(_testinternalcapi is None, "requires _testinternalcapi")
    def test_segment_map_rejects_words_outside_blocks(self):
        # The mark finds a word's object through mimalloc's segments: words
        # in a segment's metadata, in a free span or past the blocks a page
        # has handed out are not objects.
        lookup = _testinternalcapi.gc_live_lookup
        # Lists, not object(): an object() is a single header word, so op + 8
        # would be the next block.
        objs = [[i] for i in range(100)]
        op = id(objs[-1])
        self.assertEqual(lookup(op, False), op)
        self.assertEqual(lookup(op + 8, True), op)
        self.assertEqual(lookup(op + 8, False), 0)
        probes = _testinternalcapi.gc_segment_probes(objs[-1])
        for addr in probes:
            if addr:
                self.assertEqual(lookup(addr, True), 0, hex(addr))

    @unittest.skipIf(_testinternalcapi is None, "requires _testinternalcapi")
    def test_object_map_rejects_raw_blocks(self):
        # The header word is all type and flags, so a block of raw data
        # that starts with a copy of a live object's header looks like that
        # object; the allocator's object map says it is not one.
        obj = [1, 2, 3]
        self.assertEqual(_testinternalcapi.gc_live_lookup(id(obj), False),
                         id(obj))
        self.assertFalse(_testinternalcapi.gc_raw_block_is_object(obj))

    @unittest.skipIf(_testinternalcapi is None, "requires _testinternalcapi")
    def test_segment_map_follows_huge_segments(self):
        # A block larger than a 32 MiB segment gets a huge segment of its
        # own; a word past its first 32 MiB still finds it.  Freeing the
        # segment removes it from the map, so that a word into the freed
        # memory finds no segment, and a new segment at the address is
        # entered again.
        lookup = _testinternalcapi.gc_live_lookup
        segment_of = _testinternalcapi.gc_segment_of
        size = 40 << 20
        for _ in range(self.ATTEMPTS):
            big = bytes(size)
            op = id(big)
            far = op + size - 64
            self.assertEqual(lookup(far, True), op)
            self.assertNotEqual(segment_of(far), 0)
            del big
            gc.collect()
            if lookup(far, True) == 0:
                break
        self.assertEqual(lookup(far, True), 0)
        self.assertEqual(segment_of(far), 0)
        again = bytes(size)
        if id(again) <= far < id(again) + size:
            self.assertEqual(lookup(far, True), id(again))
        self.assertEqual(lookup(id(again) + size - 64, True), id(again))


class PythonFinalizationTests(unittest.TestCase):
    def test_ast_fini(self):
        # bpo-44184: Regression test for subtype_dealloc() when deallocating
        # an AST instance also destroy its AST type: subtype_dealloc() must
        # not access the type memory after deallocating the instance, since
        # the type memory can be freed as well. The test is also related to
        # _PyAST_Fini() which clears references to AST types.
        code = textwrap.dedent("""
            import ast
            import codecs
            from test import support

            # Small AST tree to keep their AST types alive
            tree = ast.parse("def f(x, y): return 2*x-y")

            # Store the tree somewhere to survive until the last GC collection
            support.late_deletion(tree)
        """)
        assert_python_ok("-c", code)

    def test_warnings_fini(self):
        # See https://github.com/python/cpython/issues/137384
        code = textwrap.dedent('''
            import asyncio
            from contextvars import ContextVar

            context_loop = ContextVar("context_loop", default=None)
            loop = asyncio.new_event_loop()
            context_loop.set(loop)
        ''')

        assert_python_ok("-c", code)


def setUpModule():
    global enabled, debug
    enabled = gc.isenabled()
    gc.disable()
    assert not gc.isenabled()
    debug = gc.get_debug()
    gc.set_debug(debug & ~gc.DEBUG_LEAK) # this test is supposed to leak
    gc.collect() # Delete 2nd generation garbage


def tearDownModule():
    gc.set_debug(debug)
    # test gc.enable() even if GC is disabled by default
    if verbose:
        print("restoring automatic collection")
    # make sure to always test gc.enable()
    gc.enable()
    assert gc.isenabled()
    if not enabled:
        gc.disable()


if __name__ == "__main__":
    unittest.main()
