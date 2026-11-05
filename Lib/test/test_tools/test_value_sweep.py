"""Tests for Tools/build/value_sweep.py, the sweep tool of the value-API
waves (pyjit/design/proposals/value-api-v2a0.md, section 1.5).

The rewrites run here without a compiler: with no AST dump, an object
argument takes the generic entry, which the tool's docstring promises is
always correct. The marks of the ownership table are checked against this
source tree's headers.
"""

import os
import tempfile
import textwrap
import unittest

from test.test_tools import imports_under_tool, skip_if_missing

skip_if_missing("build")

with imports_under_tool("build"):
    import value_sweep


TABLE = textwrap.dedent("""\
    [[row]]
    object = "PyList_Append"
    value = "PyList_AppendValue"
    family = "list"
    args = ["keep", "borrow"]
    result = "status"

    [[row]]
    object = "PyList_SET_ITEM"
    value = "PyList_SET_VALUE"
    family = "list"
    args = ["keep", "keep", "steal"]
    result = "status"

    [[row]]
    object = "PyTuple_GetItemRef"
    value = "PyTuple_GetValue"
    family = "tuple"
    args = ["keep", "keep"]
    result = "new"

    [[row]]
    object = "PyTuple_Pack"
    value = "_PyTuple_PackValues"
    family = "tuple"
    args = ["count"]
    result = "object"
    form = "pack"

    [[row]]
    object = "PyDict_Pop"
    value = "PyDict_PopValue"
    family = "dict"
    args = ["keep", "borrow", "out"]
    result = "status"

    [[row]]
    object = "PyStructSequence_SetItem"
    value = "PyStructSequence_SET_VALUE"
    family = "cell"
    args = ["keep", "keep", "steal"]
    result = "status"
    aliases = ["PyStructSequence_SET_ITEM"]

    [[constructor]]
    object = "PyLong_FromLong"
    value = "PyValue_FromLong"
    """)


def load(text=TABLE):
    with tempfile.TemporaryDirectory() as tmp:
        path = os.path.join(tmp, "table.toml")
        with open(path, "w") as f:
            f.write(text)
        return value_sweep.load_table(path)


class TableTests(unittest.TestCase):
    def test_tree_table(self):
        rows, _ = value_sweep.load_table()
        self.assertIn("PyDict_GetItemRef", rows)
        for row in rows.values():
            with self.subTest(row=row.object):
                self.assertIn(row.family,
                              ("list", "tuple", "dict", "set", "cell",
                               "number", "compare", "hash", "repr", "attr",
                               "item"))

    def test_tree_marks(self):
        # Every row's declaration in Include/ carries its mark: marking the
        # tree again changes nothing.
        rows, _ = value_sweep.load_table()
        include = os.path.join(value_sweep.SRCDIR, "Include")
        found = set()
        for root, _, files in os.walk(include):
            for name in files:
                if not name.endswith(".h"):
                    continue
                with open(os.path.join(root, name)) as f:
                    text = f.read()
                new, count = value_sweep.mark_text(text, rows.values())
                self.assertEqual(count, 0, name)
                for row in rows.values():
                    if value_sweep.declaration_pattern(row.object).search(text):
                        found.add(row.object)
        self.assertEqual(set(rows) - found, set())

    def test_bad_rows(self):
        bad_kind = TABLE.replace('args = ["keep", "borrow"]',
                                 'args = ["keep", "lend"]')
        self.assertRaises(ValueError, load, bad_kind)
        duplicate = TABLE.replace('"PyList_SET_ITEM"', '"PyList_Append"', 1)
        self.assertRaises(ValueError, load, duplicate)


class MarkTests(unittest.TestCase):
    HEADER = textwrap.dedent("""\
        PyAPI_FUNC(int) PyList_Append(PyObject *, PyObject *);
        PyAPI_FUNC(int) PyList_Insert(PyObject *, Py_ssize_t, PyObject *);
        """)

    def test_mark_and_unmark(self):
        rows, _ = load()
        marked, count = value_sweep.mark_text(self.HEADER, rows.values())
        self.assertEqual(count, 1)
        self.assertEqual(marked.splitlines()[0],
                         "Py_VALUE_COMPAT(PyList_AppendValue)")
        self.assertEqual(value_sweep.mark_text(marked, rows.values()),
                         (marked, 0))
        self.assertEqual(
            value_sweep.mark_text(marked, rows.values(), unmark=True),
            (self.HEADER, 1))


    def test_entry_mark(self):
        # A Py_VALUE_ENTRY declaration takes the mark in place of the entry.
        rows, _ = load()
        header = "Py_VALUE_ENTRY PyAPI_FUNC(int) PyList_Append(PyObject *, PyObject *);\n"
        marked, count = value_sweep.mark_text(header, rows.values())
        self.assertEqual(count, 1)
        self.assertEqual(
            marked, "Py_VALUE_COMPAT(PyList_AppendValue) PyAPI_FUNC(int) "
                    "PyList_Append(PyObject *, PyObject *);\n")
        self.assertEqual(value_sweep.mark_text(marked, rows.values()),
                         (marked, 0))
        self.assertEqual(
            value_sweep.mark_text(marked, rows.values(), unmark=True),
            (header, 1))


class ErrorTests(unittest.TestCase):
    def test_parse_errors(self):
        rows, _ = load()
        srcdir = os.path.abspath("/src")
        lines = [
            "../src/Objects/a.c:10:5: error: 'PyList_Append' is unavailable:"
            " compatibility API; use PyList_AppendValue\n",
            # GCC quotes with typographic quotes.
            "../src/Objects/b.c:3:1: error: ‘PyDict_Pop’ is"
            " unavailable: compatibility API\n",
            # The same site twice (a header in two units) is one site.
            "../src/Objects/a.c:10:5: error: 'PyList_Append' is unavailable\n",
            "../src/Objects/c.c:7:12: error: returning 'PyObject *' from a"
            " function with incompatible result type 'PyValue'\n",
            "../src/Objects/a.c:11:1: warning: unused variable 'x'\n",
        ]
        sites = value_sweep.parse_errors(lines, srcdir, "/build",
                                         value_sweep.names_of(rows))
        self.assertEqual(
            [(s["file"], s["line"], s["col"], s["function"], s["family"])
             for s in sites],
            [("Objects/a.c", 10, 5, "PyList_Append", "list"),
             ("Objects/b.c", 3, 1, "PyDict_Pop", "dict"),
             ("Objects/c.c", 7, 12, "", "")])


class RewriteTests(unittest.TestCase):
    def setUp(self):
        rows, constructors = load()
        self.rewriter = value_sweep.Rewriter(rows, constructors)

    def rewrite(self, code, name):
        """The code with the site at the first call of name rewritten, and
        the class."""
        offset = code.index(name)
        edits, klass, _ = self.rewriter.site(code, offset)
        new, rejected = value_sweep.apply_edits(code, edits)
        self.assertEqual(rejected, [])
        return new, klass

    def test_borrow(self):
        self.assertEqual(
            self.rewrite("r = PyList_Append(list, item);", "PyList_Append"),
            ("r = PyList_AppendValue(list, PyValue_FromObjectBorrow(item));",
             "M1"))
        # Singletons and _Py_ID() are known objects.
        self.assertEqual(
            self.rewrite("PyList_Append(l, Py_None);", "PyList_Append")[0],
            "PyList_AppendValue(l, _PyValue_FromObjectKnown(Py_None));")
        self.assertEqual(
            self.rewrite("PyList_Append(l, &_Py_ID(x));", "PyList_Append")[0],
            "PyList_AppendValue(l, _PyValue_FromObjectKnown(&_Py_ID(x)));")
        # An object view of a value passes the value.
        self.assertEqual(
            self.rewrite("PyList_Append(l, PyValue_AsKnownObjectBorrow(v));",
                         "PyList_Append")[0],
            "PyList_AppendValue(l, v);")

    def test_steal(self):
        # A function-like macro may take a typed pointer: cast it.
        self.assertEqual(
            self.rewrite("PyList_SET_ITEM(l, 0, item);", "PyList_SET_ITEM")[0],
            "PyList_SET_VALUE(l, 0, PyValue_PrepareOwned(_PyObject_CAST(item)));")
        self.assertEqual(
            self.rewrite("PyList_SET_ITEM(l, 0, Py_NewRef(x));",
                         "PyList_SET_ITEM")[0],
            "PyList_SET_VALUE(l, 0, "
            "PyValue_DUP(PyValue_FromObjectBorrow(_PyObject_CAST(x))));")
        # An alias takes the row's value form.
        self.assertEqual(
            self.rewrite("PyStructSequence_SET_ITEM(s, 1, x);",
                         "PyStructSequence_SET_ITEM")[0],
            "PyStructSequence_SET_VALUE(s, 1, "
            "PyValue_PrepareOwned(_PyObject_CAST(x)));")

    def test_m3_chain(self):
        # A new result passed straight to a steal: the owner passes through.
        self.assertEqual(
            self.rewrite("PyList_SET_ITEM(l, i, PyTuple_GetItemRef(t, i));",
                         "PyList_SET_ITEM"),
            ("PyList_SET_VALUE(l, i, PyTuple_GetValue(t, i));", "M3"))

    def test_pack(self):
        self.assertEqual(
            self.rewrite("t = PyTuple_Pack(2, a, Py_None);", "PyTuple_Pack")[0],
            "t = _PyTuple_PackValues(2, (PyValueBorrow[]){"
            "PyValue_FromObjectBorrow(_PyObject_CAST(a)), "
            "_PyValue_FromObjectKnown(Py_None)});")
        with self.assertRaisesRegex(value_sweep.Refuse, "count"):
            self.rewriter.site("PyTuple_Pack(n, a, b);", 0)

    def test_constructor(self):
        self.assertEqual(
            self.rewrite("return PyLong_FromLong(x);", "PyLong_FromLong"),
            ("return PyValue_FromLong(x);", "R"))
        with self.assertRaisesRegex(value_sweep.Refuse, "not returned"):
            self.rewriter.site("o = PyLong_FromLong(x);", 4)

    def test_refusals(self):
        for code, name, reason in (
            ("x = PyTuple_GetItemRef(t, 0);", "PyTuple_GetItemRef",
             "^P: new result"),
            ("PyDict_Pop(d, k, &out);", "PyDict_Pop", "^P: output"),
            ("f = &PyList_Append;", "PyList_Append", "not a call"),
            ("PyList_Append(l, a, b);", "PyList_Append", "3 arguments"),
        ):
            with self.subTest(code=code):
                with self.assertRaisesRegex(value_sweep.Refuse, reason):
                    self.rewriter.site(code, code.index(name))
        # A NULL output is the value form's NULL.
        self.assertEqual(
            self.rewrite("PyDict_Pop(d, k, NULL);", "PyDict_Pop")[0],
            "PyDict_PopValue(d, PyValue_FromObjectBorrow(k), NULL);")

    def test_macro_body(self):
        # A site reported at the use of a file-local macro rewrites the
        # body of the definition in force, through nested macros.
        code = textwrap.dedent("""\
            #define ADD(L, X) PyList_Append(L, X)
            #define ADD2(L, X) ADD(L, X)
            int f(void) { return ADD2(l, x); }
            """)
        new, klass = self.rewrite(code, "ADD2(l")
        self.assertIn("#define ADD(L, X) "
                      "PyList_AppendValue(L, PyValue_FromObjectBorrow(X))", new)
        self.assertIn("#define ADD2(L, X) ADD(L, X)", new)
        with self.assertRaisesRegex(value_sweep.Refuse, "another file"):
            self.rewriter.site("int f(void) { return OTHER(x); }",
                               len("int f(void) { return "))


class TextTests(unittest.TestCase):
    def test_enclosing_function(self):
        code = textwrap.dedent("""\
            static int
            first(int a)
            {
                return a;
            }

            /*[clinic end generated code: output=0 input=1]*/
            static PyObject *
            second_impl(PyObject *module)
            /*[clinic end generated code: output=2 input=3]*/
            {
                return NULL;
            }
            """)
        find = value_sweep.enclosing_function
        self.assertEqual(find(code, code.index("return a")), "first")
        self.assertEqual(find(code, code.index("return NULL")), "second_impl")
        self.assertIsNone(find(code, code.index("static PyObject")))

    def test_in_define(self):
        code = "#define A(x) \\\n    f(x)\nint g = f(1);\n"
        self.assertTrue(value_sweep.in_define(code, code.index("f(x)")))
        self.assertFalse(value_sweep.in_define(code, code.index("f(1)")))

    def test_scan_call(self):
        code = 'f(a, g(b, "x,)"), c[1, 2])'
        name, args, end = value_sweep.scan_call(code, 0)
        self.assertEqual(name, "f")
        self.assertEqual([code[a:b] for a, b in args],
                         ["a", 'g(b, "x,)")', "c[1, 2]"])
        self.assertEqual(end, len(code))

    def test_offset_of(self):
        code = "ab\ncd\n"
        self.assertEqual(code[value_sweep.offset_of(code, 2, 2)], "d")

    def test_apply_edits_rejects_overlaps(self):
        new, rejected = value_sweep.apply_edits(
            "abcdef", [(0, 3, "X"), (2, 4, "Y"), (4, 6, "Z")])
        self.assertEqual(new, "XdZ")
        self.assertEqual(rejected, [(2, 4, "Y")])

    def test_add_includes(self):
        with tempfile.TemporaryDirectory() as srcdir:
            internal = os.path.join(srcdir, "Include", "internal")
            os.makedirs(internal)
            for name, body in (("pycore_a.h", '#include "pycore_b.h"\n'),
                               ("pycore_b.h", ""), ("pycore_c.h", ""),
                               ("pycore_d.h", "")):
                with open(os.path.join(internal, name), "w") as f:
                    f.write(body)
            text = ('#include "Python.h"\n'
                    '#include "pycore_a.h"\n'
                    '#include "pycore_d.h"\n')
            # pycore_b.h is reached through pycore_a.h.
            new, added = value_sweep.add_includes(
                text, {"pycore_b.h": {"B"}, "pycore_c.h": {"C2", "C1"}},
                srcdir)
        self.assertEqual(added, ["pycore_c.h"])
        self.assertEqual(new.splitlines(), [
            '#include "Python.h"',
            '#include "pycore_a.h"',
            '#include "pycore_c.h"'.ljust(34) + '// C1()',
            '#include "pycore_d.h"',
        ])


if __name__ == "__main__":
    unittest.main()
