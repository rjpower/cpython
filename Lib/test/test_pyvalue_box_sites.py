"""Handlers that take an inline int or float as a value, not as a box.

Each family checks the result, the exceptions and the calls into Python code
(__hash__, __eq__ of other keys) against the object API, which these handlers
used to reach through a box. Builds with Py_STATS also check that the loops
make no box of an inline int; other builds skip that check.
"""

import os
import sys
import unittest
from test.support import import_helper

try:
    import _testcapi
except ImportError:
    _testcapi = None

N = 2000
STATS = hasattr(sys, "_stats_on")
# The dump directory of this process (Python/pystats.c).
STATS_DIR = os.environ.get("PYTHON_PYSTATS_DIR") or "/tmp/py_stats"
BOX_LINES = ("Object inline int boxes:", "Object inline float boxes:")


def empty_loop(n):
    for i in range(n):
        pass


def _stats(func, make_args):
    """The counters of one call func(*make_args()), by name (Py_STATS)."""
    func(*make_args())  # Specialise first, and let imports and caches settle.
    args = make_args()
    before = set(os.listdir(STATS_DIR)) if os.path.isdir(STATS_DIR) else set()
    sys._stats_clear()
    sys._stats_on()
    func(*args)
    sys._stats_off()
    sys._stats_dump()
    # The directory may be shared with other processes, and holds markers
    # (.run, .spawn) beside the dumps: take this process's dump.
    mine = f"Stats process: pid={os.getpid()} "
    new = []
    for name in set(os.listdir(STATS_DIR)) - before:
        if not name.endswith(".txt"):
            continue
        with open(os.path.join(STATS_DIR, name)) as stream:
            if stream.readline().startswith(mine):
                new.append(name)
    assert len(new) == 1, new
    path = os.path.join(STATS_DIR, new.pop())
    counters = {}
    with open(path) as stream:
        for line in stream:
            name, sep, value = line.strip().rpartition(":")
            if sep and value.strip().isdigit():
                counters[name.strip()] = int(value)
    os.unlink(path)
    return counters


def _count(func, make_args):
    counters = _stats(func, make_args)
    return sum(counters.get(prefix[:-1], 0) for prefix in BOX_LINES)


def executions(opcode, func, *args):
    """How many times the handler for opcode ran during func(*args)."""
    return _stats(func, lambda: args).get(f"opcode[{opcode}].execution_count", 0)


def boxes(func, *args, fresh=None):
    """The inline int and float boxes that func makes, in a Py_STATS build.

    fresh makes the arguments for each call if func consumes them. The boxes
    of an empty loop over range(N), the harness's own, are subtracted.
    """
    make_args = fresh if fresh is not None else (lambda: args)
    return _count(func, make_args) - _count(empty_loop, lambda: (N,))


class Collider:
    """Hashes as 5 and raises when compared, as a dict key that collides."""

    def __hash__(self):
        return 5

    def __eq__(self, other):
        raise RuntimeError("eq")


class Counting:
    """Hashes as 7 and records each comparison against it."""

    def __init__(self):
        self.compared = []

    def __hash__(self):
        return 7

    def __eq__(self, other):
        self.compared.append(other)
        return False


def contains_dict(d, n):
    found = 0
    for i in range(n):
        if i in d:
            found += 1
        if i not in d:
            found += 2
    return found


def contains_set(s, n):
    found = 0
    for i in range(n):
        if i in s:
            found += 1
        if i + 0.0 in s:
            found += 2
    return found


# One function per handler, each warmed on its own container type so that
# the site is specialised (CONTAINS_OP_DICT, CONTAINS_OP_SET) before the
# error paths run.
def contains_dict_one(key, container):
    return key in container


def not_contains_dict_one(key, container):
    return key not in container


def contains_set_one(key, container):
    return key in container


def not_contains_set_one(key, container):
    return key not in container


def warm(func, container):
    for _ in range(200):
        func(1, container)


def set_comprehension(n):
    return {i + 300 for i in range(n)}


def dict_comprehension(n):
    return {i + 300: i for i in range(n)}


def set_comprehension_of(items):
    return {x for x in items}


def dict_comprehension_of(items):
    return {x: 1 for x in items}


def delete_keys(d, n):
    for i in range(n):
        del d[i + 300]


def delete_one(container, key):
    del container[key]


def store_global_loop(n):
    global G
    for i in range(n):
        G = i + 300
        G = i + 0.5
    return G


def delete_global():
    global G
    del G


def compare_mixed(n):
    # An int against a float stays unspecialised.
    total = 0
    x = 0.5
    for i in range(n):
        if i < x:
            total += 1
        if x < i:
            total += 2
        if i == x:
            total += 4
        if i >= x:
            total += 8
    return total


def build_set_loop(n):
    for i in range(n):
        s = {i + 300, i + 301, i + 0.5, i + 300}
    return s


def build_set_of(a, b, c):
    return {a, b, c}


def store_name_loop(namespace, n):
    exec(STORE_NAME_CODE, namespace, namespace)


STORE_NAME_CODE = compile(
    "for i in range(%d):\n    x = 300\n    x = 0.5\n" % N,
    "<store_name>", "exec")


def store_list_loop(lst, n):
    for i in range(n):
        lst[-1] = i + 300
        lst[i % 4] = i + 0.5
        lst[-4] = i + 300


def store_list_one(lst, index, value):
    lst[index] = value


def subscript_loop(container, n):
    total = 0
    for i in range(n):
        total += container[-1]
        total += container[-4]
    return total


def subscript_one(container, index):
    return container[index]


class ContainsTests(unittest.TestCase):
    def setUp(self):
        warm(contains_dict_one, {1: 1})
        warm(not_contains_dict_one, {1: 1})
        warm(contains_set_one, {1})
        warm(not_contains_set_one, {1})

    def test_dict_results(self):
        d = {i: None for i in range(0, N, 2)}
        d[2.5] = None
        self.assertEqual(contains_dict(d, N), N // 2 * 1 + N // 2 * 2)
        for key in (0, 1, 2.5, 3.5, -1, 2**49, 2**50, 1.0, True, None):
            with self.subTest(key=key):
                self.assertEqual(contains_dict_one(key, d), d.__contains__(key))
                self.assertEqual(not_contains_dict_one(key, d), not d.__contains__(key))

    def test_dict_exceptions(self):
        d = {5: 1}
        with self.assertRaisesRegex(TypeError, "unhashable type: 'list'"):
            contains_dict_one([], d)
        with self.assertRaisesRegex(TypeError, "unhashable type: 'list'"):
            d.__contains__([])
        # An inline key that hashes like a key whose __eq__ raises.
        d = {Collider(): 1}
        for _ in range(3):
            with self.assertRaisesRegex(RuntimeError, "eq"):
                contains_dict_one(5, d)
        with self.assertRaisesRegex(RuntimeError, "eq"):
            d.__contains__(5)
        self.assertFalse(contains_dict_one(6, d))

    def test_dict_calls_eq_on_collision(self):
        other = Counting()
        d = {other: 1}
        for key in (7, 7.0):
            other.compared.clear()
            self.assertFalse(contains_dict_one(key, d))
            self.assertEqual(len(other.compared), 1)
            other.compared.clear()
            d.__contains__(key)
            self.assertEqual(len(other.compared), 1)

    def test_set_results(self):
        s = set(range(0, N, 2)) | {2.5}
        self.assertEqual(contains_set(s, N), N // 2 * 3)
        for key in (0, 1, 2.5, 3.5, -1, 2**49, 2**50, 1.0, True, None):
            with self.subTest(key=key):
                self.assertEqual(contains_set_one(key, s), s.__contains__(key))
                self.assertEqual(not_contains_set_one(key, s), not s.__contains__(key))
        self.assertEqual(contains_set_one(3, frozenset({3})), True)

    def test_set_exceptions(self):
        s = {5}
        with self.assertRaisesRegex(TypeError, "unhashable type: 'list'"):
            contains_set_one([], s)
        # A set is searched for as a frozenset, as before.
        self.assertTrue(contains_set_one({1}, {frozenset({1})}))
        self.assertTrue(contains_set_one(frozenset({1}), {frozenset({1})}))
        s = {Collider()}
        for _ in range(3):
            with self.assertRaisesRegex(RuntimeError, "eq"):
                contains_set_one(5, s)
        other = Counting()
        s = {other}
        self.assertFalse(contains_set_one(7, s))
        self.assertEqual(len(other.compared), 1)

    @unittest.skipUnless(STATS, "needs a Py_STATS build")
    def test_set_error_paths_run_the_specialised_handler(self):
        def error_paths():
            for _ in range(10):
                for key in ([], Collider(), 5, {1}):
                    try:
                        contains_set_one(key, {Collider(), frozenset({1})})
                    except (TypeError, RuntimeError):
                        pass
        self.assertGreaterEqual(executions("CONTAINS_OP_SET", error_paths), 40)

    @unittest.skipUnless(STATS, "needs a Py_STATS build")
    def test_no_box(self):
        d = {i: None for i in range(0, N, 2)}
        s = set(d)
        self.assertEqual(boxes(contains_dict, d, N), 0)
        self.assertEqual(boxes(contains_set, s, N), 0)

    @unittest.skipUnless(STATS, "needs a Py_STATS build")
    def test_box_counter_sees_boxes(self):
        # A list is searched through the object API, which boxes the key.
        items = list(range(0, 8))
        def loop(n):
            for i in range(n):
                i in items
        self.assertGreaterEqual(boxes(loop, N), N)


class AddTests(unittest.TestCase):
    def test_set_add(self):
        self.assertEqual(set_comprehension(N), set(range(300, N + 300)))
        items = [1, 2.5, 2**49, 3, 3.0, True, None, "a", (1, 2)]
        self.assertEqual(set_comprehension_of(items), set(items))
        self.assertEqual(len(set_comprehension_of([1, 1.0, True])), 1)

    def test_set_add_exceptions(self):
        with self.assertRaisesRegex(TypeError, "cannot use 'list' as a set element"):
            set_comprehension_of([1, [], 2])
        with self.assertRaisesRegex(RuntimeError, "eq"):
            set_comprehension_of([Collider(), 5])
        other = Counting()
        self.assertEqual(len(set_comprehension_of([other, 7])), 2)
        self.assertEqual(other.compared, [7])

    def test_map_add(self):
        self.assertEqual(dict_comprehension(N),
                         {i + 300: i for i in range(N)})
        items = [1, 2.5, 2**49, 3, 3.0, True, None, "a", (1, 2)]
        self.assertEqual(dict_comprehension_of(items), dict.fromkeys(items, 1))
        self.assertEqual(len(dict_comprehension_of([1, 1.0, True])), 1)

    def test_map_add_exceptions(self):
        with self.assertRaisesRegex(TypeError, "unhashable type: 'list'"):
            dict_comprehension_of([1, [], 2])
        with self.assertRaisesRegex(RuntimeError, "eq"):
            dict_comprehension_of([Collider(), 5])
        other = Counting()
        self.assertEqual(len(dict_comprehension_of([other, 7])), 2)
        self.assertEqual(other.compared, [7])

    @unittest.skipUnless(STATS, "needs a Py_STATS build")
    def test_no_box(self):
        self.assertEqual(boxes(set_comprehension, N), 0)
        self.assertEqual(boxes(dict_comprehension, N), 0)


class BuildSetTests(unittest.TestCase):
    def test_results(self):
        self.assertEqual(build_set_of(1, 2.5, 2**49), {1, 2.5, 2**49})
        self.assertEqual(build_set_of(1, 1.0, True), {1})
        self.assertEqual(build_set_of(2**50, 2**50 + 0.0, None), {2**50, None})

    def test_exceptions(self):
        with self.assertRaisesRegex(TypeError, "cannot use 'list' as a set element"):
            build_set_of(1, [], 2)
        with self.assertRaisesRegex(RuntimeError, "eq"):
            build_set_of(Collider(), 5, 6)
        other = Counting()
        self.assertEqual(len(build_set_of(other, 7, 8)), 3)
        self.assertEqual(other.compared, [7])

    @unittest.skipUnless(STATS, "needs a Py_STATS build")
    def test_no_box(self):
        self.assertEqual(boxes(build_set_loop, N), 0)


class StoreNameTests(unittest.TestCase):
    def test_results(self):
        namespace = {}
        store_name_loop(namespace, N)
        self.assertEqual((namespace["i"], namespace["x"]), (N - 1, 0.5))

        class Namespace(dict):
            def __setitem__(self, key, value):
                self.stored.append((key, value))
                super().__setitem__(key, value)

        mapping = Namespace()
        mapping.stored = []
        exec("a = 3\nb = 2.5\n", {}, mapping)
        self.assertEqual(mapping.stored, [("a", 3), ("b", 2.5)])

    @unittest.skipUnless(STATS, "needs a Py_STATS build")
    def test_no_box(self):
        self.assertEqual(boxes(store_name_loop, {}, N), 0)


class ListItemTests(unittest.TestCase):
    def test_store_results(self):
        lst = [0, 0, 0, 0]
        store_list_loop(lst, N)
        self.assertEqual(lst[-1], N - 1 + 0.5)
        for index, value in ((0, 1), (-4, 2.5), (3, 2**49), (-1, 2**50),
                             (1, None), (2, 7.5)):
            expected = [0, 0, 0, 0]
            expected.__setitem__(index, value)
            lst = [0, 0, 0, 0]
            store_list_one(lst, index, value)
            self.assertEqual(lst, expected)

    def test_store_exceptions(self):
        for index in (4, -5, 2**49, -2**49):
            with self.subTest(index=index):
                with self.assertRaisesRegex(IndexError, "list assignment index out of range"):
                    store_list_one([0, 0, 0, 0], index, 1)
        with self.assertRaisesRegex(TypeError, "list indices must be integers"):
            store_list_one([0], 0.5, 1)
        calls = []

        class L(list):
            def __setitem__(self, index, value):
                calls.append((index, value))

        store_list_one(L([1]), 0, 5)
        self.assertEqual(calls, [(0, 5)])

    def test_subscript_results(self):
        for container in ([1, 2.5, 2**49, 2**50], (1, 2.5, 2**49, 2**50),
                          [None, 3, 4, 5]):
            for index in range(-5, 6):
                with self.subTest(container=container, index=index):
                    try:
                        expected = container.__getitem__(index)
                    except IndexError as error:
                        with self.assertRaises(IndexError) as caught:
                            subscript_one(container, index)
                        self.assertEqual(str(caught.exception), str(error))
                    else:
                        self.assertIs(subscript_one(container, index), expected)
        self.assertEqual(subscript_loop([1, 2.5, 3, 4], 10), 10 * (4 + 1))
        self.assertEqual(subscript_loop((1, 2, 3, 4), 10), 10 * (4 + 1))

    def test_subscript_exceptions(self):
        for index in (2**49, -2**49, 2**50):
            with self.assertRaisesRegex(IndexError, "index out of range"):
                subscript_one([1], index)
            with self.assertRaisesRegex(IndexError, "index out of range"):
                subscript_one((1,), index)
        calls = []

        class T(tuple):
            def __getitem__(self, index):
                calls.append(index)
                return "sub"

        self.assertEqual(subscript_one(T((1,)), 0), "sub")
        self.assertEqual(calls, [0])

    @unittest.skipUnless(STATS, "needs a Py_STATS build")
    def test_no_box(self):
        self.assertEqual(boxes(store_list_loop, [0, 0, 0, 0], N), 0)
        self.assertEqual(boxes(subscript_loop, [1, 2.5, 3, 4], N), 0)
        self.assertEqual(boxes(subscript_loop, (1, 2.5, 3, 4), N), 0)


GLOBALS_CODE = (
    "def read_global():\n    return X\n"
    "def set_global(value):\n    global X\n    X = value\n"
    "def del_global():\n    global X\n    del X\n")


def make_namespace(**names):
    namespace = {"__builtins__": __builtins__}
    namespace.update(names)
    exec(GLOBALS_CODE, namespace)
    return namespace


@unittest.skipIf(_testcapi is None, "needs _testcapi")
class DictWatcherTests(unittest.TestCase):
    def events_of(self, namespace, action):
        from test.support import gc_collect
        gc_collect()
        wid = _testcapi.add_dict_watcher(0)
        try:
            _testcapi.watch_dict(wid, namespace)
            action()
            return list(_testcapi.get_dict_watcher_events())
        finally:
            _testcapi.clear_dict_watcher(wid)

    def test_store_global(self):
        namespace = make_namespace()
        def action():
            namespace["set_global"](3)
            namespace["set_global"](2.5)
            namespace["set_global"](3)
            namespace["set_global"](2**60)
            namespace["del_global"]()
        self.assertEqual(self.events_of(namespace, action),
                         ["new:X:3", "mod:X:2.5", "mod:X:3",
                          f"mod:X:{2**60}", "del:X"])

    def test_store_name(self):
        namespace = {}
        def action():
            exec("a = 3\na = 4.5\na = 3\n", {}, namespace)
        self.assertEqual(self.events_of(namespace, action),
                         ["new:a:3", "mod:a:4.5", "mod:a:3"])

    def test_delete_subscr(self):
        d = {5: 1, 2.5: 2, 7: 3}
        def action():
            delete_one(d, 5)
            delete_one(d, 2.5)
            delete_one(d, 7.0)
        self.assertEqual(self.events_of(d, action), ["del:5", "del:2.5", "del:7.0"])


class SpecialisedGlobalTests(unittest.TestCase):
    """A LOAD_GLOBAL that was specialised reads the new state of the dict."""

    def warmed(self, namespace):
        read = namespace["read_global"]
        for _ in range(200):
            read()
        return read

    def test_store_global(self):
        namespace = make_namespace(X=1)
        read = self.warmed(namespace)
        for value in (3, 2.5, 2**49, 2**60, -7, 3):
            namespace["set_global"](value)
            self.assertEqual(read(), value)
            self.assertIs(type(read()), type(value))

    def test_store_name(self):
        namespace = make_namespace(X=1)
        read = self.warmed(namespace)
        for value in (3, 2.5, 2**60):
            exec(f"X = {value!r}", namespace)
            self.assertEqual(read(), value)

    def test_delete_subscr(self):
        namespace = make_namespace(X=1)
        read = self.warmed(namespace)
        delete_one(namespace, "X")
        with self.assertRaises(NameError):
            read()
        namespace["set_global"](4)
        self.assertEqual(read(), 4)
        namespace["del_global"]()
        with self.assertRaises(NameError):
            read()

    def test_map_add(self):
        # A namespace built by a dict comprehension holds inline values.
        pairs = [("__builtins__", __builtins__), ("X", 3)]
        namespace = {key: value for key, value in pairs}
        exec(GLOBALS_CODE, namespace)
        read = self.warmed(namespace)
        self.assertEqual(read(), 3)
        namespace["set_global"](4.5)
        self.assertEqual(read(), 4.5)


class DeleteTests(unittest.TestCase):
    def test_delete_results(self):
        d = {i + 300: i for i in range(N)}
        delete_keys(d, N)
        self.assertEqual(d, {})
        d = {1: 1, 2.5: 2}
        delete_one(d, 1.0)
        delete_one(d, 2.5)
        self.assertEqual(d, {})
        lst = [1, 2, 3]
        delete_one(lst, 1)
        self.assertEqual(lst, [1, 3])
        delete_one(lst, -1)
        self.assertEqual(lst, [1])

    def test_delete_exceptions(self):
        d = {}
        for key in (3, 2.5, 2**49):
            with self.subTest(key=key):
                with self.assertRaises(KeyError) as caught:
                    delete_one(d, key)
                self.assertEqual(caught.exception.args, (key,))
        with self.assertRaisesRegex(TypeError, "unhashable type: 'list'"):
            delete_one({}, [])
        with self.assertRaisesRegex(RuntimeError, "eq"):
            delete_one({Collider(): 1}, 5)
        with self.assertRaisesRegex(TypeError, "does not support item deletion"):
            delete_one(5, 1)
        with self.assertRaisesRegex(TypeError, "does not support item deletion"):
            delete_one(2.5, 1)
        with self.assertRaises(IndexError):
            delete_one([], 1)

    def test_dict_subclass_delitem_is_called(self):
        calls = []

        class D(dict):
            def __delitem__(self, key):
                calls.append(key)

        delete_one(D({1: 1}), 1)
        self.assertEqual(calls, [1])

    @unittest.skipUnless(STATS, "needs a Py_STATS build")
    def test_no_box(self):
        def fresh():
            return {i + 300: i for i in range(N)}, N
        self.assertEqual(boxes(delete_keys, fresh=fresh), 0)


class StoreGlobalTests(unittest.TestCase):
    def tearDown(self):
        globals().pop("G", None)

    def test_store_and_delete(self):
        self.assertEqual(store_global_loop(N), 0.5 + N - 1)
        self.assertEqual(G, N - 1 + 0.5)
        delete_global()
        self.assertNotIn("G", globals())
        with self.assertRaisesRegex(NameError, "name 'G' is not defined"):
            delete_global()

    def test_dict_subclass_globals(self):
        class Globals(dict):
            def __setitem__(self, key, value):
                if key != "f":
                    raise AssertionError("a global store must not call this")
                super().__setitem__(key, value)

        namespace = Globals(__builtins__=__builtins__)
        exec("def f(v):\n global X\n X = v\n", namespace)
        namespace["f"](3)
        namespace["f"](2.5)
        self.assertEqual(dict.__getitem__(namespace, "X"), 2.5)

    @unittest.skipUnless(STATS, "needs a Py_STATS build")
    def test_no_box(self):
        self.assertEqual(boxes(store_global_loop, N), 0)


class CompareTests(unittest.TestCase):
    def test_mixed_results(self):
        # i < 0.5 only for 0; 0.5 < i for i >= 1; never equal; i >= 0.5 for i >= 1
        self.assertEqual(compare_mixed(N), 1 + 2 * (N - 1) + 8 * (N - 1))

    def test_inline_pairs_match_the_object_api(self):
        values = [0, 1, -1, 2**49, -2**49, 0.0, -0.0, 0.5, 1.0, 2.0**49,
                  float("inf"), float("-inf"), float("nan"), 2**50, 2**60,
                  # The int51 boundary and the floats around it
                  2**50 - 1, -2**50, -(2**50 - 1), -(2**50) - 1,
                  float(2**50 - 1), float(-2**50), float(2**50),
                  2.0**50 - 0.5, -(2.0**50) + 0.5, 2**53 + 1, 5e-324]
        ops = {
            "<": lambda a, b: a < b, "<=": lambda a, b: a <= b,
            "==": lambda a, b: a == b, "!=": lambda a, b: a != b,
            ">": lambda a, b: a > b, ">=": lambda a, b: a >= b,
        }
        methods = {"<": "__lt__", "<=": "__le__", "==": "__eq__",
                   "!=": "__ne__", ">": "__gt__", ">=": "__ge__"}
        reflected = {"<": "__gt__", "<=": "__ge__", "==": "__eq__",
                     "!=": "__ne__", ">": "__lt__", ">=": "__le__"}
        for name, op in ops.items():
            for a in values:
                for b in values:
                    result = getattr(a, methods[name])(b)
                    if result is NotImplemented:
                        result = getattr(b, reflected[name])(a)
                    with self.subTest(op=name, a=a, b=b):
                        self.assertIs(op(a, b), result)
                        # And again once the site has been warmed.
                        for _ in range(3):
                            self.assertIs(op(a, b), result)

    def test_compare_to_objects_runs_python(self):
        calls = []

        class C:
            def __gt__(self, other):
                calls.append(other)
                return True

            def __eq__(self, other):
                raise RuntimeError("eq")

        def lt(a, b):
            return a < b

        def eq(a, b):
            return a == b

        self.assertTrue(lt(3, C()))
        self.assertTrue(lt(2.5, C()))
        self.assertEqual(calls, [3, 2.5])
        with self.assertRaisesRegex(RuntimeError, "eq"):
            eq(3, C())
        with self.assertRaisesRegex(TypeError, "not supported between"):
            lt(3, "a")

    @unittest.skipUnless(STATS, "needs a Py_STATS build")
    def test_no_box(self):
        self.assertEqual(boxes(compare_mixed, N), 0)


class Dunders:
    """A Python class whose dunders take their operands and return None."""

    def __add__(self, o): return None
    def __radd__(self, o): return None
    def __iadd__(self, o): return self
    def __lt__(self, o): return True
    def __contains__(self, o): return True
    def __getitem__(self, k): return None
    def __setitem__(self, k, v): pass
    def __delitem__(self, k): pass
    def __setattr__(self, name, v): pass


class Descr:
    def __get__(self, obj, owner=None): return None
    def __set__(self, obj, v): pass


class HasDescr:
    d = Descr()


def loop_of(call):
    """A loop of N iterations of call(a, i); i is inline."""
    def loop(n):
        for i in range(n):
            call(i)
    return loop


class PythonClassSlotTests(unittest.TestCase):
    """The value slot_* functions pass an inline operand to a dunder.

    Each case calls a value dispatcher from C with a Python-class operand
    and an inline int. The call into the C function boxes each inline
    argument (the harness's own boxes), and the slot adds none.
    """

    def setUp(self):
        _testinternalcapi = import_helper.import_module('_testinternalcapi')
        self.dispatch = _testinternalcapi.value_dispatch
        self.container = _testinternalcapi.value_container
        self.setattr = _testinternalcapi.value_setattr

    def test_results(self):
        a = Dunders()
        self.assertEqual(self.dispatch("Add", a, 1), (None, False))
        self.assertEqual(self.dispatch("Add", 1, a), (None, False))
        self.assertEqual(self.dispatch("Add", 2.5, a), (None, False))
        self.assertEqual(self.dispatch("InPlaceAdd", a, 1)[0], a)
        self.assertTrue(self.dispatch("RichCompareBool", a, 3, 0))
        self.assertIsNone(self.container("getitem", a, 3))
        self.assertIsNone(self.container("setitem", a, 3, 4))
        self.assertIsNone(self.container("delitem", a, 3))
        self.assertTrue(self.container("contains", a, 3))
        self.setattr(a, "x", 5)
        self.setattr(HasDescr(), "d", 5)

    @unittest.skipUnless(STATS, "needs a Py_STATS build")
    def test_no_box(self):
        a = Dunders()
        h = HasDescr()
        d, c, sa = self.dispatch, self.container, self.setattr
        # (name, call, boxes that the call itself makes: each inline argument,
        # and value_setattr's METH_VARARGS parser boxes its value again)
        cases = [
            ("add", lambda i: d("Add", a, i), 1),
            ("radd", lambda i: d("Add", i, a), 1),
            ("inplace", lambda i: d("InPlaceAdd", a, i), 1),
            ("compare", lambda i: d("RichCompareBool", a, i, 0), 2),
            ("getitem", lambda i: c("getitem", a, i), 1),
            ("setitem", lambda i: c("setitem", a, i, i), 2),
            ("delitem", lambda i: c("delitem", a, i), 1),
            ("contains", lambda i: c("contains", a, i), 1),
            ("setattr", lambda i: sa(a, "x", i), 2),
            ("descr_set", lambda i: sa(h, "d", i), 2),
        ]
        for name, call, inline_args in cases:
            with self.subTest(case=name):
                self.assertEqual(boxes(loop_of(call), N), N * inline_args)


def import_loop(n):
    for i in range(n):
        import os
        from os import path


def get_attr_loop(n, a):
    for i in range(n):
        a.real
        a.imag


class AttributeAndImportTests(unittest.TestCase):
    def test_results(self):
        self.assertIsNone(import_loop(3))
        for a in (3, 2.5, True):
            self.assertIsNone(get_attr_loop(3, a))

    @unittest.skipUnless(STATS, "needs a Py_STATS build")
    def test_import_makes_no_box(self):
        # The level of IMPORT_NAME is an inline int
        self.assertEqual(boxes(import_loop, N), 0)

    @unittest.skipUnless(STATS, "needs a Py_STATS build")
    def test_attributes_of_inline_values_make_no_box(self):
        for a in (3, 2.5, True):
            with self.subTest(operand=a):
                self.assertEqual(boxes(get_attr_loop, N, a), 0)


if __name__ == "__main__":
    unittest.main()
