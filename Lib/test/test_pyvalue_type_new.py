"""Type constructor input validation, delegation and retained owner views."""

import unittest


class TypeNewReferenceControls(unittest.TestCase):
    def test_empty_bases_and_namespace_copy(self):
        token = object()
        namespace = {"__module__": __name__, "value": token, "number": 2147483647}
        made = type("OwnerType", (), namespace)
        self.assertEqual(made.__bases__, (object,))
        self.assertIs(made.value, token)
        self.assertEqual(made.number, 2147483647)
        namespace["value"] = None
        self.assertIs(made.value, token)

    def test_explicit_base_and_field_subclasses(self):
        class Name(str):
            pass

        class Bases(tuple):
            def __getitem__(self, index):
                raise AssertionError("constructor called tuple override")

        class Namespace(dict):
            pass

        class Base:
            inherited = 3.25

        made = type.__new__(type, Name("OwnerType"), Bases((Base,)),
                            Namespace(value=None))
        self.assertEqual(made.__name__, "OwnerType")
        self.assertEqual(made.__bases__, (Base,))
        self.assertEqual(made.inherited, 3.25)
        self.assertIsNone(made.value)

    def test_exact_argument_count_errors(self):
        for args in ((), (None,), (None, None), (None, None, None, None)):
            with self.subTest(count=len(args)), self.assertRaises(TypeError) as caught:
                type.__new__(type, *args)
            self.assertEqual(str(caught.exception),
                             "type.__new__() takes exactly 3 arguments "
                             f"({len(args)} given)")

    def test_ordered_invalid_inline_fields(self):
        cases = (
            ((2147483647, 3.25, None),
             "type.__new__() argument 1 must be str, not int"),
            ((None, 3.25, None),
             "type.__new__() argument 1 must be str, not None"),
            (("OwnerType", 3.25, None),
             "type.__new__() argument 2 must be tuple, not float"),
            (("OwnerType", None, None),
             "type.__new__() argument 2 must be tuple, not None"),
        )
        for args, message in cases:
            with self.subTest(args=args), self.assertRaises(TypeError) as caught:
                type.__new__(type, *args)
            self.assertEqual(str(caught.exception), message)
        with self.assertRaisesRegex(TypeError, r"argument 3 must be dict"):
            type.__new__(type, "OwnerType", (), 2147483647)

    def test_field_error_type_name_precision(self):
        # The U/O! parser diagnostics historically limit the encoded type name
        # to 50 bytes, rather than 50 Unicode characters.
        for name, displayed in (("A" * 80, "A" * 50), ("é" * 40, "é" * 25)):
            invalid = type(name, (), {})()
            with self.subTest(name=name), self.assertRaises(TypeError) as caught:
                type.__new__(type, invalid, (), {})
            self.assertEqual(str(caught.exception),
                             "type.__new__() argument 1 must be str, not " + displayed)

    def test_metaclass_one_argument_and_builtin_fast_path(self):
        class Meta(type):
            pass

        self.assertIs(type(5), int)
        with self.assertRaises(TypeError) as caught:
            Meta(5)
        self.assertEqual(str(caught.exception),
                         "type.__new__() takes exactly 3 arguments (1 given)")

    def test_winning_metaclass_receives_original_inputs(self):
        calls = []

        class Winner(type):
            def __new__(mcls, name, bases, namespace, **kwargs):
                calls.append((name, bases, namespace, kwargs))
                return super().__new__(mcls, name, bases, namespace, **kwargs)

        class Base(metaclass=Winner):
            pass

        calls.clear()
        bases = (Base,)
        namespace = {"__module__": __name__, "value": 2147483647}
        made = type.__new__(type, "OwnerType", bases, namespace)
        self.assertIs(type(made), Winner)
        name, seen_bases, seen_namespace, kwargs = calls.pop()
        self.assertEqual(name, "OwnerType")
        self.assertIs(seen_bases, bases)
        self.assertIs(seen_namespace, namespace)
        self.assertEqual(kwargs, {})
        self.assertEqual(calls, [])

    def test_delegated_non_type_result_is_owned(self):
        result = object()

        class Winner(type):
            def __new__(mcls, name, bases, namespace):
                if name == "Delegated":
                    return result
                return super().__new__(mcls, name, bases, namespace)

        class Base(metaclass=Winner):
            pass

        self.assertIs(type.__new__(type, "Delegated", (Base,), {}), result)

    def test_winning_metaclass_error_keeps_identity(self):
        error = LookupError("metaclass primary")
        calls = []

        class Winner(type):
            def __new__(mcls, name, bases, namespace):
                if name == "Delegated":
                    calls.append(name)
                    raise error
                return super().__new__(mcls, name, bases, namespace)

        class Base(metaclass=Winner):
            pass

        with self.assertRaises(LookupError) as caught:
            type.__new__(type, "Delegated", (Base,), {})
        self.assertIs(caught.exception, error)
        self.assertEqual(calls, ["Delegated"])

    def test_keywords_and_descriptor_callbacks(self):
        events = []
        marker = object()

        class Base:
            def __init_subclass__(cls, *, selected):
                events.append(("init_subclass", selected, cls.field))

        class Descriptor:
            def __set_name__(self, owner, name):
                events.append(("set_name", name, owner.__name__))

        descriptor = Descriptor()
        made = type("OwnerType", (Base,), {"field": descriptor}, selected=marker)
        self.assertIs(made.field, descriptor)
        self.assertEqual(events, [("set_name", "field", "OwnerType"),
                                  ("init_subclass", marker, descriptor)])

    def test_init_subclass_error_keeps_identity(self):
        error = ValueError("init subclass primary")
        events = []

        class Base:
            def __init_subclass__(cls, **kwargs):
                events.append(kwargs)
                raise error

        with self.assertRaises(ValueError) as caught:
            type("OwnerType", (Base,), {}, selected=2147483647)
        self.assertIs(caught.exception, error)
        self.assertEqual(events, [{"selected": 2147483647}])

    def test_set_name_error_keeps_identity(self):
        error = RuntimeError("set name primary")
        calls = []

        class Descriptor:
            def __set_name__(self, owner, name):
                calls.append(name)
                raise error

        with self.assertRaises(RuntimeError) as caught:
            type("OwnerType", (), {"field": Descriptor()})
        self.assertIs(caught.exception, error)
        self.assertEqual(calls, ["field"])


def candidate_call_state(code):
    import _testinternalcapi
    state = _testinternalcapi.native_scope_call_state(code)
    _, _, nlocals, base, count, _, values, present, strong = state
    if present & ~strong:
        raise AssertionError("native input home lacks an owner")
    if not (nlocals <= base and base + count <= len(values)):
        raise AssertionError("invalid native call window")
    return state


class TypeNewCandidateTests(unittest.TestCase):
    def test_ordinary_and_expanded_constructor_homes(self):
        for expanded in (False, True):
            states = []
            marker = object()

            class Base:
                def __init_subclass__(cls, *, selected):
                    self.assertIs(selected, marker)
                    states.append(candidate_call_state(driver.__code__))

            class Descriptor:
                def __set_name__(self, owner, name):
                    states.append(candidate_call_state(driver.__code__))

            namespace = {"field": Descriptor(), "value": 2147483647}
            bases = (Base,)
            args = ("Observed", bases, namespace)
            target = type
            if expanded:
                def driver():
                    return target(*args, selected=marker)
            else:
                def driver():
                    return target("Observed", bases, namespace, selected=marker)

            made = driver()
            self.assertEqual(made.value, 2147483647)
            self.assertEqual(len(states), 2)
            for state in states:
                base, values = state[3], state[6]
                self.assertIs(values[base], target)
                if expanded:
                    self.assertEqual(state[4], 4)
                    self.assertEqual(values[base + 2], args)
                else:
                    self.assertEqual(values[base + 2], "Observed")
                    self.assertIs(values[base + 3], bases)
                    self.assertIs(values[base + 4], namespace)

    def test_delegation_observes_original_call_homes(self):
        states = []

        class Winner(type):
            def __new__(mcls, name, bases, namespace):
                if name == "Delegated":
                    states.append(candidate_call_state(driver.__code__))
                return super().__new__(mcls, name, bases, namespace)

        class Base(metaclass=Winner):
            pass

        namespace = {"value": 3.25}
        target = type.__new__

        def driver():
            return target(type, "Delegated", (Base,), namespace)

        made = driver()
        self.assertIs(type(made), Winner)
        self.assertEqual(made.value, 3.25)
        state, = states
        self.assertIs(state[6][state[3]], target)
        self.assertIs(state[6][state[3] + 5], namespace)

    def test_frozendict_namespace(self):
        import builtins
        namespace = builtins.frozendict({"__module__": __name__, "value": None})
        made = type.__new__(type, "FrozenNamespace", (), namespace)
        self.assertIsNone(made.value)
        self.assertIs(type(made), type)


if __name__ == "__main__":
    unittest.main()
