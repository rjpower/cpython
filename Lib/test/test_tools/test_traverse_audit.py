"""Tests for Tools/boil/traverse_audit.py."""

import os
import subprocess
import sys
import tempfile
import textwrap
import unittest

from test.test_tools import skip_if_missing, toolsdir

skip_if_missing("boil")

SCRIPT = os.path.join(toolsdir, "boil", "traverse_audit.py")

HEADER = """\
typedef struct {
    void *buf;
    PyObject *obj;
    Py_ssize_t len;
} Py_buffer;

typedef struct {
    PyObject_HEAD
    Py_ssize_t ma_used;
    PyDictKeysObject *ma_keys;
} PyDictObject;
"""

# An accessor of the kind Include/cpython/classobject.h defines.
ACCESSOR_HEADER = """\
typedef struct {
    PyObject_HEAD
    PyObject *func;
    PyObject *forgotten;
} AccessorObject;

static inline PyObject* Fixture_GET_FUNC(PyObject *op) {
    return ((AccessorObject *)op)->func;
}
#define Fixture_GET_FUNC(op) Fixture_GET_FUNC(_PyObject_CAST(op))
"""

# A helper in a header of the module's directory, as sre_lib.h's.
LIB_HEADER = """\
static void
lib_fini(LibObject *self)
{
    Py_XDECREF(self->held);
}
"""

# The shape of _testbuffer's ndarray: the object reaches a PyMem-allocated
# list of nodes, each of which embeds a Py_buffer whose obj is a reference.
SOURCE = """\
struct ndbuf;
typedef struct ndbuf {
    struct ndbuf *next;
    struct ndbuf *prev;
    char *data;
    Py_buffer base;
} ndbuf_t;

typedef struct {
    PyObject_HEAD
    int flags;
    ndbuf_t *head;
} NDArrayObject;

static PyTypeObject NDArray_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fixture.ndarray",
    sizeof(NDArrayObject),
    0,
    ndarray_dealloc,
};

/* A typedef separate from the definition, an array of pointers, and two
   levels of indirection, the second of which is the limit. */
typedef struct item item_t;
typedef struct inner inner_t;
typedef struct deep deep_t;

struct deep {
    PyObject *too_deep;
};

struct inner {
    PyObject *payload;
    deep_t *deep;
};

struct item {
    PyObject *value;
    inner_t *inner;
};

typedef struct {
    PyObject_HEAD
    item_t **items;
    item_t inline_items[4];
} TableObject;

static PyTypeObject Table_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fixture.table",
    sizeof(TableObject),
};

/* A traverse that walks the list but forgets a member that dealloc
   releases. */
struct link {
    struct link *next;
    PyObject *key;
    PyObject *value;
};

typedef struct {
    PyObject_HEAD
    struct link *first;
} ChainObject;

static int
chain_traverse(PyObject *op, visitproc visit, void *arg)
{
    ChainObject *self = (ChainObject *)op;
    for (struct link *l = self->first; l != NULL; l = l->next) {
        Py_VISIT(l->key);
    }
    return 0;
}

static void
chain_dealloc(PyObject *op)
{
    ChainObject *self = (ChainObject *)op;
    struct link *l = self->first;
    while (l != NULL) {
        struct link *next = l->next;
        Py_DECREF(l->key);
        Py_DECREF(l->value);
        PyMem_Free(l);
        l = next;
    }
}

static PyTypeObject Chain_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fixture.chain",
    .tp_basicsize = sizeof(ChainObject),
    .tp_dealloc = chain_dealloc,
    .tp_traverse = chain_traverse,
};

/* A traverse that visits through a macro, and a dealloc that releases
   through a helper; a pointer to a module state is borrowed. */
struct pair {
    PyObject *key;
};

typedef struct {
    PyObject *cached;
} fixture_state;

typedef struct {
    PyObject_HEAD
    struct pair *pair;
    PyObject *other;
    fixture_state *state;
} HelperObject;

#define VISIT_PAIR(p) do { if (p) Py_VISIT((p)->key); } while (0)

static int
helper_traverse(PyObject *op, visitproc visit, void *arg)
{
    VISIT_PAIR(((HelperObject *)op)->pair);
    return 0;
}

static int
helper_clear(PyObject *op)
{
    HelperObject *self = (HelperObject *)op;
    Py_CLEAR(self->pair->key);
    Py_CLEAR(self->other);
    return 0;
}

static void
helper_dealloc(PyObject *op)
{
    (void)helper_clear(op);
}

static PyTypeObject Helper_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fixture.helper",
    .tp_basicsize = sizeof(HelperObject),
    .tp_dealloc = helper_dealloc,
    .tp_traverse = helper_traverse,
};

/* Bases given at run time. */
typedef struct {
    PyDictObject dict;
    PyObject *extra;
} SubDictObject;

static PyTypeObject SubDict_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fixture.subdict",
    sizeof(SubDictObject),
};

typedef struct {
    PyObject_HEAD
    PyObject *args;
} ErrorObject;

static PyType_Spec error_spec = {
    .name = "fixture.error",
    .basicsize = sizeof(ErrorObject),
};

/* A struct of the same name in another file, which that file traverses. */
typedef struct {
    PyObject_HEAD
    PyObject *x_attr;
} XxoObject;

static PyTypeObject Xxo_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "fixture.xxo",
    sizeof(XxoObject),
};

static int
fixture_exec(PyObject *m)
{
    SubDict_Type.tp_base = &PyDict_Type;
    PyObject *error = PyType_FromSpecWithBases(&error_spec, PyExc_Exception);
    return error == NULL ? -1 : 0;
}

/* A pointer to an object struct is a reference already, not a struct to
   follow. */
typedef struct {
    PyObject_HEAD
    NDArrayObject *owner;
} OwnerObject;

static int
owner_traverse(PyObject *op, visitproc visit, void *arg)
{
    Py_VISIT(((OwnerObject *)op)->owner);
    return 0;
}

static PyTypeObject Owner_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fixture.owner",
    .tp_basicsize = sizeof(OwnerObject),
    .tp_traverse = owner_traverse,
};

/* Report 3.  "const" begins the name of a member, not its type. */
typedef struct {
    PyObject_HEAD
    PyObject *constraints;
    PyObject *unnamed;
} ConstObject;

static int
const_traverse(PyObject *op, visitproc visit, void *arg)
{
    Py_VISIT(((ConstObject *)op)->constraints);
    return 0;
}

static PyTypeObject Const_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fixture.const",
    .tp_basicsize = sizeof(ConstObject),
    .tp_traverse = const_traverse,
};

/* A release in a function whose return type is a macro on the line
   before, as _sre's LOCAL(void) state_fini. */
typedef struct {
    PyObject_HEAD
    PyObject *string;
} StateObject;

#define LOCAL(type) static inline type

LOCAL(void)
state_fini(StateObject *self)
{
    Py_XDECREF(self->string);
}

static int
state_traverse(PyObject *op, visitproc visit, void *arg)
{
    return 0;
}

static void
state_dealloc(PyObject *op)
{
    state_fini((StateObject *)op);
}

static PyTypeObject State_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fixture.state",
    .tp_basicsize = sizeof(StateObject),
    .tp_dealloc = state_dealloc,
    .tp_traverse = state_traverse,
};

/* An accessor defined in Include/, and a release in a helper defined in
   a header of this directory. */
static int
accessor_traverse(PyObject *op, visitproc visit, void *arg)
{
    Py_VISIT(Fixture_GET_FUNC(op));
    return 0;
}

static PyTypeObject Accessor_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fixture.accessor",
    .tp_basicsize = sizeof(AccessorObject),
    .tp_traverse = accessor_traverse,
};

typedef struct {
    PyObject_HEAD
    PyObject *held;
} LibObject;

static int
lib_traverse(PyObject *op, visitproc visit, void *arg)
{
    return 0;
}

static void
lib_dealloc(PyObject *op)
{
    lib_fini((LibObject *)op);
}

static PyTypeObject Lib_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fixture.lib",
    .tp_basicsize = sizeof(LibObject),
    .tp_dealloc = lib_dealloc,
    .tp_traverse = lib_traverse,
};

/* A visit callback handed to an enumerator, as _lsprof's visit_entry. */
struct entry {
    PyObject *payload;
};

typedef struct {
    PyObject_HEAD
    struct entry *current;
    PyObject *loose;
} TreeObject;

static int
visit_entry(struct entry *e, void *arg)
{
    return ((visitproc)arg)(e->payload, NULL);
}

static int
tree_traverse(PyObject *op, visitproc visit, void *arg)
{
    return Enumerate(((TreeObject *)op)->current, visit_entry, visit);
}

static PyTypeObject Tree_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fixture.tree",
    .tp_basicsize = sizeof(TreeObject),
    .tp_traverse = tree_traverse,
};

/* Delegation to the base's traverse, as defaultdict's; a delegate the
   tool cannot resolve credits nothing. */
static int
dict_traverse(PyObject *op, visitproc visit, void *arg)
{
    Py_VISIT(((PyDictObject *)op)->ma_keys);
    return 0;
}

PyTypeObject PyDict_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "dict",
    .tp_basicsize = sizeof(PyDictObject),
    .tp_traverse = dict_traverse,
};

typedef struct {
    PyDictObject dict;
    PyObject *factory;
    PyObject *forgotten;
} DefDictObject;

static int
defdict_traverse(PyObject *op, visitproc visit, void *arg)
{
    Py_VISIT(((DefDictObject *)op)->factory);
    return PyDict_Type.tp_traverse(op, visit, arg);
}

static PyTypeObject DefDict_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fixture.defdict",
    .tp_basicsize = sizeof(DefDictObject),
    .tp_traverse = defdict_traverse,
};

static int
otherdict_traverse(PyObject *op, visitproc visit, void *arg)
{
    Py_VISIT(((DefDictObject *)op)->factory);
    Py_VISIT(((DefDictObject *)op)->forgotten);
    return ((PyTypeObject *)Unknown)->tp_traverse(op, visit, arg);
}

static PyTypeObject OtherDict_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fixture.otherdict",
    .tp_basicsize = sizeof(DefDictObject),
    .tp_traverse = otherdict_traverse,
};
"""

OTHER = """\
typedef struct {
    PyObject_HEAD
    PyObject *x_attr;
} XxoObject;

static int
xxo_traverse(PyObject *op, visitproc visit, void *arg)
{
    Py_VISIT(((XxoObject *)op)->x_attr);
    return 0;
}

static PyTypeObject Xxo_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "other.xxo",
    .tp_basicsize = sizeof(XxoObject),
    .tp_traverse = xxo_traverse,
};
"""


def run_audit(root, *args, check=True):
    proc = subprocess.run([sys.executable, SCRIPT, "--root", root, *args],
                          capture_output=True, text=True, check=check)
    return proc


def report_line(output, name):
    for line in output.splitlines():
        if f": {name} (" in line:
            return line
    return None


class TraverseAuditTests(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.tmp.cleanup)
        root = cls.root = cls.tmp.name
        os.mkdir(os.path.join(root, "Include"))
        os.mkdir(os.path.join(root, "Modules"))
        with open(os.path.join(root, "Include", "pybuffer.h"), "w") as f:
            f.write(textwrap.dedent(HEADER))
        with open(os.path.join(root, "Include", "accessor.h"), "w") as f:
            f.write(ACCESSOR_HEADER)
        with open(os.path.join(root, "Modules", "fixture_lib.h"), "w") as f:
            f.write(LIB_HEADER)
        with open(os.path.join(root, "Modules", "fixture.c"), "w") as f:
            f.write(textwrap.dedent(SOURCE))
        with open(os.path.join(root, "Modules", "other.c"), "w") as f:
            f.write(textwrap.dedent(OTHER))
        cls.output = run_audit(root).stdout
        missing, _, partial = cls.output.partition("misses members")
        cls.missing, _, cls.inherited = missing.partition("probably inherited")
        cls.partial, _, cls.unvisited = partial.partition(
            "does not name a member")

    def test_pointer_to_struct_is_followed(self):
        line = report_line(self.missing, "fixture.ndarray")
        self.assertIsNotNone(line, self.output)
        self.assertIn("head->base.obj", line)
        # The list's own links add nothing the head does not.
        self.assertNotIn("next", line)
        self.assertNotIn("prev", line)

    def test_typedef_arrays_and_depth(self):
        line = report_line(self.missing, "fixture.table")
        self.assertIsNotNone(line, self.output)
        refs = line.split("): ", 1)[1].split(", ")
        self.assertEqual(sorted(refs), sorted([
            "items[]->value",
            "items[]->inner->payload",
            "inline_items[].value",
            "inline_items[].inner->payload",
            "inline_items[].inner->deep->too_deep",
        ]))

    def test_partial_traverse_through_pointer(self):
        line = report_line(self.partial, "fixture.chain")
        self.assertIsNotNone(line, self.output)
        self.assertTrue(line.endswith(": first->value"), line)

    def test_macros_and_helpers(self):
        line = report_line(self.partial, "fixture.helper")
        self.assertIsNotNone(line, self.output)
        self.assertTrue(line.endswith(": other"), line)

    def test_runtime_bases(self):
        for name in ("fixture.subdict", "fixture.error"):
            with self.subTest(name=name):
                self.assertIsNone(report_line(self.missing, name), self.output)
                self.assertIsNotNone(report_line(self.inherited, name),
                                     self.output)

    def test_struct_shared_only_within_a_file(self):
        line = report_line(self.missing, "fixture.xxo")
        self.assertIsNotNone(line, self.output)
        self.assertTrue(line.endswith(": x_attr"), line)

    def check(self, *entries):
        allow = os.path.join(self.root, "allow.txt")
        with open(allow, "w") as f:
            f.write("# comment\n" + "".join(e + "\n" for e in entries))
        return run_audit(self.root, "--check", "--allow", allow, check=False)

    # Every type the fixture reports with no traverse or a partial one.
    LISTED = [
        "Modules/fixture.c NDArray_Type: reason",
        "Modules/fixture.c Table_Type: reason",
        "Modules/fixture.c Chain_Type: reason",
        "Modules/fixture.c Helper_Type: reason",
        "Modules/fixture.c Xxo_Type: reason",
        "Modules/fixture.c State_Type: reason",
        "Modules/fixture.c Lib_Type: reason",
    ]

    def test_check_passes_when_all_listed(self):
        proc = self.check(*self.LISTED)
        self.assertEqual(proc.returncode, 0, proc.stdout)
        self.assertEqual(proc.stdout, "")

    def test_check_fails_on_unlisted(self):
        proc = self.check(*self.LISTED[1:])
        self.assertEqual(proc.returncode, 1, proc.stdout)
        self.assertIn("fixture.ndarray has no tp_traverse", proc.stdout)
        proc = self.check(*self.LISTED[:2], *self.LISTED[3:])
        self.assertEqual(proc.returncode, 1, proc.stdout)
        self.assertIn("fixture.chain has a partial tp_traverse", proc.stdout)

    def test_check_fails_on_stale_or_malformed_entry(self):
        proc = self.check(*self.LISTED, "Modules/fixture.c Owner_Type: reason")
        self.assertEqual(proc.returncode, 1, proc.stdout)
        self.assertIn("Owner_Type is not reported any more", proc.stdout)
        proc = self.check(*self.LISTED, "Modules/fixture.c Owner_Type")
        self.assertEqual(proc.returncode, 1, proc.stdout)
        self.assertIn("not \"path variable: reason\"", proc.stdout)

    def test_const_inside_a_name(self):
        line = report_line(self.unvisited, "fixture.const")
        self.assertIsNotNone(line, self.output)
        self.assertTrue(line.endswith(": unnamed"), line)

    def test_return_type_macro_on_the_line_before(self):
        line = report_line(self.partial, "fixture.state")
        self.assertIsNotNone(line, self.output)
        self.assertTrue(line.endswith(": string"), line)
        self.assertIsNone(report_line(self.unvisited, "fixture.state"),
                          self.output)

    def test_header_accessors_and_helpers(self):
        line = report_line(self.unvisited, "fixture.accessor")
        self.assertIsNotNone(line, self.output)
        self.assertTrue(line.endswith(": forgotten"), line)
        line = report_line(self.partial, "fixture.lib")
        self.assertIsNotNone(line, self.output)
        self.assertTrue(line.endswith(": held"), line)

    def test_function_reference(self):
        line = report_line(self.unvisited, "fixture.tree")
        self.assertIsNotNone(line, self.output)
        self.assertTrue(line.endswith(": loose"), line)

    def test_delegation_to_base_traverse(self):
        line = report_line(self.unvisited, "fixture.defdict")
        self.assertIsNotNone(line, self.output)
        self.assertTrue(line.endswith(": forgotten"), line)
        line = report_line(self.unvisited, "fixture.otherdict")
        self.assertIsNotNone(line, self.output)
        self.assertTrue(line.endswith(": ma_keys"), line)

    def test_pointer_to_object_struct(self):
        self.assertIsNone(report_line(self.output, "fixture.owner"),
                          self.output)


if __name__ == "__main__":
    unittest.main()
