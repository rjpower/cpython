"""Tests for Tools/build/gc_inline_audit.py, the inline audit of precise C
frames.  Each fixture is compiled into a scratch object by the configured
compiler, never into the interpreter, and the audit runs over it."""

import os
import shlex
import shutil
import subprocess
import sysconfig
import tempfile
import textwrap
import unittest
from pathlib import Path

from test import support
from test.test_tools import imports_under_tool, skip_if_missing

skip_if_missing("build")
with imports_under_tool("build"):
    import gc_inline_audit

CC = shlex.split(sysconfig.get_config_var("CC") or "")

PRELUDE = """\
#define Py_GC_PRECISE __attribute__((section("py_gc_precise")))
typedef struct _object PyObject;
extern void collect(void);
extern PyObject *make(int x);
extern int use(PyObject *o);
static inline __attribute__((always_inline)) int
opaque(int x) { PyObject *o = make(x); collect(); return use(o); }
static inline __attribute__((always_inline)) int
unmarked(int x) { collect(); return x + 1; }
static inline __attribute__((always_inline)) int
pure(int x) { return x * 2; }
Py_GC_PRECISE static inline __attribute__((always_inline)) int
checked(int x) { collect(); return x; }
"""

GOOD = "Py_GC_PRECISE int good(int x) { return checked(pure(x)); }\n"
BAD = "Py_GC_PRECISE int bad(int x) { return opaque(x); }\n"


@unittest.skipUnless(not support.MS_WINDOWS and CC and shutil.which(CC[0])
                     and shutil.which("readelf"),
                     "requires the configured C compiler and readelf")
class InlineAuditTests(unittest.TestCase):

    def setUp(self):
        self.tmp = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.tmp)

    def build(self, name, src, *flags):
        c = os.path.join(self.tmp, name + ".c")
        with open(c, "w") as f:
            f.write(PRELUDE + textwrap.dedent(src))
        obj = os.path.join(self.tmp, name + ".o")
        subprocess.run([*CC, *flags, "-c", c, "-o", obj], check=True)
        return obj, c

    def audit(self, obj, *sources):
        return gc_inline_audit.audit(obj, [Path(s) for s in sources])

    def test_checked_inlines_pass(self):
        for opt in ("-O0", "-O3"):
            with self.subTest(opt):
                obj, c = self.build("good" + opt, GOOD, "-g", opt)
                self.assertEqual(self.audit(obj, c), [])

    def test_opaque_inline_fails(self):
        for opt in ("-O0", "-O3"):
            with self.subTest(opt):
                obj, c = self.build("bad" + opt, GOOD + BAD, "-g", opt)
                errors = self.audit(obj, c)
                self.assertEqual(len(errors), 1, errors)
                self.assertIn("precise 'bad' inlines 'opaque', whose body "
                              "fails the C mode", errors[0])
                self.assertIn("'o' is held only in a C local", errors[0])

    def test_unmarked_inline_that_checks_clean_passes(self):
        # Its locals live in the precise frame, and it holds none across
        # its collection (10a-2.md, rule 6).
        src = GOOD + "Py_GC_PRECISE int calls(int x) { return unmarked(x); }\n"
        for opt in ("-O0", "-O3"):
            with self.subTest(opt):
                obj, c = self.build("unmarked" + opt, src, "-g", opt)
                self.assertEqual(self.audit(obj, c), [])

    def test_no_debug_info_fails(self):
        obj, c = self.build("nog", GOOD, "-O2")
        errors = self.audit(obj, c)
        self.assertEqual(len(errors), 1, errors)
        self.assertIn("no .debug_info", errors[0])

    def test_cold_part_and_clone_fail(self):
        # Named as the compiler names a cold part and a clone outside the
        # section.
        src = GOOD + """
            void good_cold(void) __asm__("good.cold");
            void good_cold(void) { collect(); }
            int good_isra(int) __asm__("good.isra.0");
            int good_isra(int x) { return x; }
        """
        obj, c = self.build("cold", src, "-g", "-O2")
        errors = self.audit(obj, c)
        self.assertEqual(len(errors), 2, errors)
        self.assertIn("'good.cold' is a cold part", errors[0] + errors[1])
        self.assertIn("'good.isra.0', a clone of precise 'good'",
                      errors[0] + errors[1])

    def test_precise_symbol_without_dwarf_fails(self):
        nog, c1 = self.build("nodwarf", "Py_GC_PRECISE int lost(int x) "
                             "{ return x; }\n", "-O2")
        withg, c2 = self.build("withg", GOOD, "-g", "-O2")
        both = os.path.join(self.tmp, "both.o")
        subprocess.run([*CC, "-r", "-nostdlib", nog, withg, "-o", both],
                       check=True)
        errors = self.audit(both, c1, c2)
        self.assertEqual(len(errors), 1, errors)
        self.assertIn("'lost' is in section py_gc_precise but has no DWARF",
                      errors[0])

    def test_preserve_none_section_is_audited(self):
        # The tail-call handlers' section (Py_GC_PRECISE_PRESERVE_NONE)
        src = GOOD + ('__attribute__((section("py_gc_precise_pn"))) '
                      "int handler(int x) { return opaque(x); }\n")
        obj, c = self.build("pn", src, "-g", "-O2")
        errors = self.audit(obj, c)
        self.assertEqual(len(errors), 1, errors)
        self.assertIn("precise 'handler' inlines 'opaque'", errors[0])

    @unittest.skipUnless(shutil.which("clang-21") or shutil.which("clang"),
                         "requires clang")
    def test_linked_image_with_indexed_addresses(self):
        # clang's DWARF 5 gives a function's low_pc as an index into
        # .debug_addr; in a linked image the audit places the function by
        # it.
        clang = shutil.which("clang-21") or shutil.which("clang")
        c = os.path.join(self.tmp, "image.c")
        with open(c, "w") as f:
            f.write(PRELUDE + GOOD + BAD)
        image = os.path.join(self.tmp, "image.so")
        subprocess.run([clang, "-gdwarf-5", "-O2", "-fPIC", "-shared", c,
                        "-o", image], check=True)
        errors = self.audit(image, c)
        self.assertEqual(len(errors), 1, errors)
        self.assertIn("precise 'bad' inlines 'opaque'", errors[0])

    def test_marked_headers_are_sources(self):
        # A static inline function marked in a header is precise where it
        # is inlined, so the audit reads the header's markers: the internal
        # headers, the public ones and the core's private ones.
        sources = gc_inline_audit.marked_sources()
        names = [p.name for p in sources]
        for header in ("pycore_gc_walk.h", "pyvalue.h", "ceval_macros.h"):
            self.assertIn(header, names)

    OUTLINED = """
        static __attribute__((noinline)) int
        outlined(int x) { collect(); return x; }
        Py_GC_PRECISE static __attribute__((noinline)) int
        outlined_marked(int x) { collect(); return x; }
        static __attribute__((noinline)) int
        outlined_pure(int x) { return x + 3; }
        static __attribute__((noinline)) int
        borrow_across_bare(int x) { collect(); return x; }
    """

    def build_image(self, name, src):
        c = os.path.join(self.tmp, name + ".c")
        with open(c, "w") as f:
            f.write(PRELUDE + textwrap.dedent(src))
        image = os.path.join(self.tmp, name + ".so")
        subprocess.run([*CC, "-g", "-O0", "-fPIC", "-shared", c, "-o", image],
                       check=True)
        return image, c

    @unittest.skipUnless(shutil.which("objdump"), "requires objdump")
    def test_outlined_static_callee_fails(self):
        # A static callee that this build leaves outlined has a frame of its
        # own, which the walk scans: it must be precise, or listed with the
        # reason its frame may be opaque.
        image, c = self.build_image("outlined", self.OUTLINED + """
            Py_GC_PRECISE int caller(int x) { return outlined(x); }
        """)
        errors = self.audit(image, c)
        self.assertEqual(len(errors), 1, errors)
        self.assertIn("static 'outlined', which precise 'caller' calls, is "
                      "outlined and collecting", errors[0])

    @unittest.skipUnless(shutil.which("objdump"), "requires objdump")
    def test_outlined_marked_pure_or_listed_callee_passes(self):
        image, c = self.build_image("outlined_ok", self.OUTLINED + """
            Py_GC_PRECISE int caller(int x)
            {
                return outlined_marked(x) + outlined_pure(x)
                       + borrow_across_bare(x);
            }
            int unmarked_caller(int x) { return outlined(x); }
        """)
        self.assertIn("borrow_across_bare", gc_inline_audit.OUTLINED_OPAQUE)
        self.assertEqual(self.audit(image, c), [])

    def test_cli_exit_status(self):
        obj, c = self.build("cli", GOOD + BAD, "-g", "-O2")
        with support.captured_stdout() as out:
            self.assertEqual(gc_inline_audit.main([obj, c]), 1)
        self.assertIn("inlines 'opaque'", out.getvalue())
        obj, c = self.build("cli_good", GOOD, "-g", "-O2")
        with support.captured_stdout():
            self.assertEqual(gc_inline_audit.main([obj, c]), 0)


if __name__ == "__main__":
    unittest.main()
