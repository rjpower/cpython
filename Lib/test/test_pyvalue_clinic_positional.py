"""Tuple-backed positional constructor controls and candidate owner witnesses."""

import itertools
import types
import unittest


def code_arguments(code):
    return [
        code.co_argcount, code.co_posonlyargcount, code.co_kwonlyargcount,
        code.co_nlocals, code.co_stacksize, code.co_flags, code.co_code,
        code.co_consts, code.co_names, code.co_varnames, code.co_filename,
        code.co_name, code.co_qualname, code.co_firstlineno, code.co_linetable,
        code.co_exceptiontable,
    ]


def sample():
    return 42


def candidate_state(code):
    import _testinternalcapi
    state = _testinternalcapi.native_scope_call_state(code)
    _, _, nlocals, base, count, _, values, present, strong = state
    if present & ~strong:
        raise AssertionError("live native call home lacks an owner")
    if not (nlocals <= base and base + count <= len(values)):
        raise AssertionError("invalid native call window")
    return state


class PositionalReferenceControls(unittest.TestCase):
    def test_code_required_and_optional_arguments(self):
        code = sample.__code__
        for optional in ([], [code.co_freevars],
                         [code.co_freevars, code.co_cellvars]):
            with self.subTest(optional=len(optional)):
                cloned = types.CodeType(*(code_arguments(code) + optional))
                self.assertEqual(types.FunctionType(cloned, {})(), 42)
                self.assertEqual(cloned.co_freevars, code.co_freevars)
                self.assertEqual(cloned.co_cellvars, code.co_cellvars)

    def test_code_ordinary_and_expanded(self):
        args = code_arguments(sample.__code__)
        ordinary = types.CodeType(
            args[0], args[1], args[2], args[3], args[4], args[5], args[6],
            args[7], args[8], args[9], args[10], args[11], args[12], args[13],
            args[14], args[15])
        expanded = types.CodeType(*args)
        self.assertEqual(ordinary, expanded)

    def test_code_shape_precedes_index_conversion(self):
        events = []

        class Index:
            def __index__(self):
                events.append("index")
                return 0

        args = code_arguments(sample.__code__)
        args[0] = Index()
        # Too few, and more than the constructor's optional arguments take.
        for invalid in (args[:15], args + [None] * 8):
            with self.assertRaises(TypeError):
                types.CodeType(*invalid)
        with self.assertRaisesRegex(TypeError, "keyword"):
            types.CodeType(*args, invalid=1)
        self.assertEqual(events, [])

    def test_code_index_conversion_and_error(self):
        events = []
        error = ValueError("index primary")

        class Index:
            def __index__(self):
                events.append("index")
                return 0

        args = code_arguments(sample.__code__)
        args[0] = Index()
        self.assertEqual(types.FunctionType(types.CodeType(*args), {})(), 42)
        self.assertEqual(events, ["index"])

        class BadIndex:
            def __index__(self):
                raise error

        args[0] = BadIndex()
        with self.assertRaises(ValueError) as caught:
            types.CodeType(*args)
        self.assertIs(caught.exception, error)

    def test_code_converter_type_validation(self):
        args = code_arguments(sample.__code__)
        for index, value in ((6, "bytes required"), (7, []), (10, 1), (15, None)):
            invalid = args.copy()
            invalid[index] = value
            with self.subTest(index=index), self.assertRaises(TypeError):
                types.CodeType(*invalid)

    def test_method_constructor_routes(self):
        receiver = object()

        def function(self):
            return self

        ordinary = types.MethodType(function, receiver)
        expanded = types.MethodType(*(function, receiver))
        self.assertIs(ordinary(), receiver)
        self.assertIs(expanded(), receiver)
        with self.assertRaises(TypeError):
            types.MethodType(function)
        with self.assertRaises(TypeError):
            types.MethodType(function, receiver, extra=1)

    def test_itertools_registered_constructor_routes(self):
        cases = (
            (itertools.pairwise, ([1, 2, 3],), [(1, 2), (2, 3)]),
            (itertools.dropwhile, (lambda x: x < 2, [1, 2, 3]), [2, 3]),
            (itertools.takewhile, (lambda x: x < 3, [1, 2, 3]), [1, 2]),
            (itertools.filterfalse, (None, [0, 1, 0]), [0, 0]),
            (itertools.starmap, (pow, [(2, 3), (3, 2)]), [8, 9]),
        )
        for target, args, expected in cases:
            with self.subTest(target=target):
                self.assertEqual(list(target(*args)), expected)
                with self.assertRaises(TypeError):
                    target(*args, invalid=1)
        self.assertEqual(list(itertools.pairwise([1, 2, 3])), [(1, 2), (2, 3)])
        self.assertEqual(list(itertools.dropwhile(lambda x: x < 2, [1, 2])), [2])

    def test_list_init_slot_and_subtype_keyword_policy(self):
        value = []
        list.__init__(value, (1, 2))
        self.assertEqual(value, [1, 2])
        list.__init__(value)
        self.assertEqual(value, [])
        with self.assertRaises(TypeError):
            list.__init__(value, [1], invalid=1)

        class Subclass(list):
            def __new__(cls, *args, **kwargs):
                return list.__new__(cls)

            def __init__(self, *args, **kwargs):
                pass

        value = Subclass()
        list.__init__(value, (3, 4), invalid=1)
        self.assertEqual(value, [3, 4])


class PositionalCandidateTests(unittest.TestCase):
    """Requires this fork's registered constructors and real frame observer."""

    @classmethod
    def setUpClass(cls):
        import _testclinic
        cls.clinic = _testclinic

    def check_constructor(self, target, expanded, *, default=False, warm=False):
        events, states = [], []
        pending = []

        class Value:
            def visit(self, index, tail):
                pending.clear()
                states.append(candidate_state(driver.__code__))
                events.append(("visit", index, tail))
                self.alive = True
                self.nested = self_outer.clinic.OwnerEcho(2147483647)

            def __repr__(self):
                if not self.alive or self.nested != 2147483647:
                    raise AssertionError("positional input lost across callback")
                events.append("repr")
                return "alive"

        class Index:
            def __index__(self):
                states.append(candidate_state(driver.__code__))
                events.append("index")
                return 2147483647

        self_outer = self
        value = Value()
        pending.append(value)
        index = Index()
        args = (value,) if default else (value, index, 2147483647)
        if expanded:
            def driver():
                return target(*args)
        elif default:
            def driver():
                return target(value)
        else:
            def driver():
                return target(value, index, 2147483647)

        count = 200 if warm else 1
        for _ in range(count):
            result = driver()
            if target is self.clinic.PositionalOwnerNew:
                self.assertEqual(result, "alive")
            else:
                self.assertIs(type(result), target)
        expected = [("visit", 7, None), "repr"] if default else [
            "index", ("visit", 2147483647, 2147483647), "repr"]
        self.assertEqual(events, expected * count)
        self.assertEqual(pending, [])
        for state in states:
            base, values = state[3], state[6]
            self.assertIs(values[base], target)
            if expanded:
                self.assertEqual(state[4], 4)
                self.assertIs(values[base + 2], args)
            else:
                self.assertIs(values[base + 2], value)
        if warm:
            import _testinternalcapi
            names = [row[0] for row in
                     _testinternalcapi.get_ir(driver.__code__, True)]
            self.assertIn("CALL_NO_SELF_NON_PY_GENERAL", names)

    def test_new_ordinary_and_expanded(self):
        for expanded in (False, True):
            with self.subTest(expanded=expanded):
                self.check_constructor(self.clinic.PositionalOwnerNew, expanded)

    def test_init_ordinary_and_expanded(self):
        for expanded in (False, True):
            with self.subTest(expanded=expanded):
                self.check_constructor(self.clinic.PositionalOwnerInit, expanded)

    def test_default_and_warm_routes(self):
        self.check_constructor(self.clinic.PositionalOwnerNew, False, default=True)
        for target in (self.clinic.PositionalOwnerNew,
                       self.clinic.PositionalOwnerInit):
            self.check_constructor(target, False, warm=True)

    def test_shape_failure_does_not_convert(self):
        events = []

        class Index:
            def __index__(self):
                events.append("index")
                return 1

        for target in (self.clinic.PositionalOwnerNew,
                       self.clinic.PositionalOwnerInit):
            with self.assertRaisesRegex(TypeError, "keyword"):
                target(object(), Index(), invalid=1)
            with self.assertRaises(TypeError):
                target(object(), Index(), None, None)
        self.assertEqual(events, [])

    def test_converter_and_implementation_errors(self):
        index_error = ValueError("index primary")
        visit_error = LookupError("visit primary")
        events = []

        class BadIndex:
            def __index__(self):
                raise index_error

        class Value:
            def visit(self, index, tail):
                events.append("visit")
                raise visit_error

            def __repr__(self):
                raise AssertionError("repr after failed visit")

        for target in (self.clinic.PositionalOwnerNew,
                       self.clinic.PositionalOwnerInit):
            with self.assertRaises(ValueError) as caught:
                target(Value(), BadIndex())
            self.assertIs(caught.exception, index_error)
            with self.assertRaises(LookupError) as caught:
                target(Value(), 1)
            self.assertIs(caught.exception, visit_error)
        self.assertEqual(events, ["visit", "visit"])

    def test_code_feedback_optional_argument(self):
        code = sample.__code__
        cloned = types.CodeType(*(code_arguments(code) + [
            code.co_freevars, code.co_cellvars, code.co_fbinit]))
        self.assertEqual(cloned.co_fbinit, code.co_fbinit)
        self.assertEqual(types.FunctionType(cloned, {})(), 42)


if __name__ == "__main__":
    unittest.main()
