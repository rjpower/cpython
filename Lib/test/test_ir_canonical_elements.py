"""Canonical element ownership comes from checked guards and an atomic DUP."""

import io
import gc
from dataclasses import replace
from pathlib import Path
import sys
import tempfile
import types
import unittest
import weakref
from unittest import mock

from test import support, test_tools

test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import analyzer
    import cwriter
    import ir_generator
    import ir_numbers


class CanonicalElementTests(unittest.TestCase):
    def source(self):
        return (Path(__file__).resolve().parents[2] / "Python/bytecodes.c").read_text()

    def emit(self, source, name, record=False):
        numbers = ir_numbers.load_table()
        if name == "RENAMED_ELEMENT":
            numbers = ir_numbers.NumberTable(
                numbers={**numbers.numbers, name: numbers.numbers["BINARY_OP_SUBSCR_LIST_INT"]},
                constants=numbers.constants)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "source.c"
            path.write_text(source)
            analysis = analyzer.analyze_files([str(path)], numbers=numbers)
        formats = ir_generator.compute_formats(analysis)
        out = io.StringIO()
        writer = cwriter.CWriter(out, 0, False)
        emitter = ir_generator.IREmitter(writer, analysis, formats)
        instruction = ir_generator.handler_instructions(analysis)[name]
        ir_generator.generate_handler(name, instruction, formats[name], emitter, writer, record)
        return out.getvalue()

    def test_list_and_tuple_use_the_same_canonical_token_consumer(self):
        source = self.source()
        for kind in ("LIST", "TUPLE"):
            for record in (False, True):
                with self.subTest(kind=kind, record=record):
                    body = self.emit(source, f"BINARY_OP_SUBSCR_{kind}_INT", record)
                    self.assertIn(f"PyValue_XDUP(Py{kind.title()}_GET_VALUE(", body)
                    self.assertIn("IR_STORE_CANONICAL_TEMP_DST(IR_A(ir_word), res);", body)
                    self.assertIn("IR_STORE_CANONICAL_DST(IR_A(ir_word), res);", body)
                    self.assertNotIn("IR_STORE_TEMP_DST(", body)
                    self.assertNotIn("IR_STORE_DST(", body)
                    self.assertLess(body.index("IR_STORE_CANONICAL_TEMP_DST("),
                                    body.index("_PyValue_XCLOSE_POINTER(ir_own_B);"))
                    self.assertLess(body.index("index >= Py"), body.index("PyValue_XDUP(Py"))
        renamed = source.replace("BINARY_OP_SUBSCR_LIST_INT", "RENAMED_ELEMENT")
        self.assertIn("IR_STORE_CANONICAL_DST(", self.emit(renamed, "RENAMED_ELEMENT"))

    def test_guards_bounds_and_anchor_must_match_current_values(self):
        source = self.source()
        guard = "IR_GUARD_ELEMENT_INDEX(list_st, sub_st, index, FROM_END);"
        read = "res = IR_DUP_CANONICAL_ELEMENT(list_st, index);"
        mutations = (
            ("IR_GUARD_INLINE_INT(value);", "(void)value;"),
            ("IR_GUARD_ELEMENT_TYPE(container, LIST);", "(void)container;"),
            (guard, guard.replace("FROM_END", "NONNEGATIVE")),
            (guard, guard.replace("sub_st", "list_st")),
            (read, read.replace("list_st", "sub_st")),
            (read, "index++; " + read),
            (read, "if (++index) {} " + read),
            (read, "if ((index = 0)) {} " + read),
            (read, "if ((list_st = sub_st, 0)) {} " + read),
            (read, "while (++index < 0) {} " + read),
            (read, "for (index++; 0; index++) {} " + read),
            (read, "opaque(); " + read),
            (read, "PyValue_CLOSE(list_st); " + read),
        )
        for old, new in mutations:
            with self.subTest(new=new), self.assertRaises(SyntaxError):
                self.emit(source.replace(old, new, 1), "BINARY_OP_SUBSCR_LIST_INT")

    def test_native_lowering_names_cannot_be_replaced_by_source(self):
        source = self.source()
        guard = "IR_GUARD_INLINE_INT(value);"
        for prefix in ("PyValue (*PyValue_XDUP)(PyValue) = replacement; ",
                       "#define PyList_GET_VALUE replacement\n",
                       "#if 1\n"):
            suffix = "\n#endif" if prefix.startswith("#if") else ""
            with self.subTest(prefix=prefix), self.assertRaises(SyntaxError):
                self.emit(source.replace(guard, prefix + guard + suffix, 1),
                          "BINARY_OP_SUBSCR_LIST_INT")

    def test_result_mutation_cannot_reuse_an_old_canonical_token(self):
        source = self.source()
        read = "res = IR_DUP_CANONICAL_ELEMENT(list_st, index);"
        for suffix in (" res = unknown_value;", " opaque(&res);",
                       " PyValue alias = opaque(&(res));",
                       " if ((res = unknown_value)) {}",
                       " while ((res = unknown_value, 0)) {}",
                       " for (; (res = unknown_value, 0);) {}",
                       " PyValue *alias = &(res); opaque(alias);",
                       " PyValue alias = res; PyValue_CLOSE(res);"
                       " if (oparg) { alias = PyValue_None; } res = alias;"):
            with self.subTest(suffix=suffix):
                try:
                    body = self.emit(source.replace(read, read + suffix, 1),
                                     "BINARY_OP_SUBSCR_LIST_INT")
                except SyntaxError:
                    continue
                self.assertNotIn("IR_STORE_CANONICAL_DST(", body)

    def test_list_iteration_publishes_the_checked_element_without_reclassification(self):
        source = self.source()
        for record in (False, True):
            with self.subTest(record=record):
                body = self.emit(source, "FOR_ITER_LIST", record)
                self.assertIn("PyValue_XDUP(PyValue_Borrow(_PyList_ITEMS(", body)
                self.assertNotIn("PyList_GET_VALUE(", body)
                self.assertIn("IR_PUBLISH_CANONICAL_TEMP(window[2], next);", body)
                self.assertIn("IR_PUBLISH_TEMP(window[1], null_or_index);", body)
                self.assertIn("IR_MOVE_TO_DST_OR_DISCARD(IR_A(ir_word), window[2]);", body)
                acquire = body.index("PyValue_XDUP(PyValue_Borrow(_PyList_ITEMS(")
                self.assertLess(body.index("_PyEval_CompleteForIter("), acquire)
                self.assertLess(body.index("if (!PyValue_IsTaggedIndex("), acquire)
                self.assertLess(body.index("if ((size_t)index >="), acquire)
        helper = ir_generator.HELPERS["IR_DUP_CANONICAL_ELEMENT"]
        with mock.patch.dict(ir_generator.HELPERS, {
                "IR_DUP_CANONICAL_ELEMENT": replace(helper, result_owned=False)}):
            body = self.emit(source, "FOR_ITER_LIST")
            self.assertNotIn("IR_PUBLISH_CANONICAL_TEMP(window[2]", body)

    def test_iteration_guards_and_current_result_facts_expire(self):
        source = self.source()
        guard = "IR_GUARD_ELEMENT_INDEX(iter, null_or_index, index, TAGGED);"
        read = "next = IR_DUP_CANONICAL_ELEMENT(iter, index);"
        for mutation in (
                "index++; " + read,
                "if (++index) {} " + read,
                "if ((iter = null_or_index, 0)) {} " + read,
                "opaque(&iter); " + read,
                "opaque(); " + read):
            with self.subTest(mutation=mutation), self.assertRaises(SyntaxError):
                self.emit(source.replace(read, mutation, 1), "FOR_ITER_LIST")
        for mutation in (
                read + " next = unknown_value;",
                read + " if ((next = unknown_value, 0)) {}",
                read + " if (oparg) { next = unknown_value; }",
                read + " while ((next = unknown_value, 0)) {}",
                read + " next = unknown_value; while (0) { next = PyValue_None; }",
                read + " next = unknown_value; for (int j = 0; j < 0; j++) { next = PyValue_None; }"):
            with self.subTest(mutation=mutation):
                body = self.emit(source.replace(read, mutation, 1), "FOR_ITER_LIST")
                self.assertNotIn("IR_PUBLISH_CANONICAL_TEMP(window[2]", body)
        for callback in ("opaque(&next);", "opaque();"):
            body = self.emit(source.replace(read, read + " " + callback, 1), "FOR_ITER_LIST")
            # The normal callback spill can publish the checked value before
            # calling out. It cannot certify a later use of changed C bits.
            self.assertLess(body.index("IR_PUBLISH_CANONICAL_TEMP(window[2]"),
                            body.index(callback))
            self.assertNotIn("IR_PUBLISH_CANONICAL_TEMP(window[2]",
                             body[body.index(callback):])
        for replacement in (guard.replace("TAGGED", "FROM_END"), "(void)iter;"):
            with self.subTest(replacement=replacement), self.assertRaises(SyntaxError):
                self.emit(source.replace(guard, replacement, 1), "FOR_ITER_LIST")
        type_guard = "IR_GUARD_ELEMENT_TYPE(iter, LIST);"
        for loop in ("while (0)", "for (int j = 0; j < 0; j++)"):
            with self.subTest(loop=loop), self.assertRaises(SyntaxError):
                self.emit(source.replace(type_guard, loop + " " + type_guard, 1), "FOR_ITER_LIST")

    def test_iteration_prefix_cannot_shadow_a_native_lowering_operation(self):
        source = self.source()
        prefix = "op(_ITER_CHECK_LIST, (iter, null_or_index -- iter, null_or_index)) {"
        for declaration in (
                "PyObject *(PyValue_AsKnownObjectBorrow)(PyValueBorrow);",
                "PyObject * const PyValue_AsKnownObjectBorrow = replacement;",
                "PyObject *(*PyValue_AsKnownObjectBorrow)(PyValueBorrow) = replacement;",
                "volatile Py_ssize_t (*PyList_GET_SIZE)(PyObject *) = replacement;",
                '#include "alias.h"\n',
                '#pragma push_macro("hidden_alias")\n',
                "#define PyValue_AsKnownObjectBorrow replacement\n"):
            with self.subTest(declaration=declaration), self.assertRaises(SyntaxError):
                self.emit(source.replace(prefix, prefix + declaration, 1), "FOR_ITER_LIST")

    def test_only_append_bodies_select_the_direct_fast_boundary(self):
        source = self.source()
        for name in ("LIST_APPEND", "CALL_LIST_APPEND", "CALL_NO_SELF_LIST_APPEND"):
            for record in (False, True):
                with self.subTest(name=name, record=record):
                    body = self.emit(source, name, record)
                    call = body.index("int err = _PyList_AppendTakeValueFast(")
                    self.assertNotIn("_PyList_AppendTakeValue(", body)
                    self.assertLess(body.index("PyValue item = "), call)
                    self.assertIn("PROCESS_ERROR_AT(", body[call:])
                    if name != "LIST_APPEND":
                        self.assertLess(body.index("LOCK_OBJECT("), call)
                        self.assertLess(call, body.index("UNLOCK_OBJECT("))


class CanonicalElementRuntimeTests(unittest.TestCase):
    def test_specialized_bounds_values_and_generic_misses(self):
        import _testinternalcapi

        def template(container, index):
            return container[index]

        values = (None, True, 37, 1 << 80, 1.5, float("nan"), object())
        for kind in (list, tuple):
            read = types.FunctionType(template.__code__.replace(), globals())
            container = kind(values)
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 10):
                self.assertIs(read(container, 0), values[0])
            expected = f"BINARY_OP_SUBSCR_{kind.__name__.upper()}_INT"
            if kind is tuple or not support.Py_GIL_DISABLED:
                self.assertIn(expected, {word[0] for word in
                                        _testinternalcapi.get_ir(read.__code__, True)})
            for index, value in enumerate(values):
                result = read(container, index)
                if isinstance(value, float) and value != value:
                    self.assertNotEqual(result, result)
                elif isinstance(value, (int, float)):
                    self.assertEqual(result, value)
                else:
                    self.assertIs(result, value)
            self.assertIs(read(container, -1), values[-1])
            for index in (len(values), -len(values) - 1, 1 << 80):
                with self.subTest(kind=kind, index=index), self.assertRaises(IndexError):
                    read(container, index)

            calls = []

            class Override(kind):
                def __getitem__(self, index):
                    calls.append(index)
                    return values[-1]

            self.assertIs(read(Override(values), 0), values[-1])
            self.assertEqual(calls, [0])

    def test_temporary_anchor_finalizer_observes_published_result(self):
        import _testinternalcapi

        def read(factory):
            mutable = 0
            result = factory()[0]
            return result, mutable

        class Payload:
            pass

        events = []

        class Retired:
            def __del__(self):
                frame = sys._getframe(1)
                while frame is not None and frame.f_code is not read.__code__:
                    frame = frame.f_back
                if frame is None:
                    events.append(("missing frame",))
                    return
                frame.f_locals["mutable"] = 42
                gc.collect()
                events.append(tuple(_testinternalcapi.gc_verify()))

        for kind in (list, tuple):
            def factory():
                return kind((Payload(), Retired()))

            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 10):
                result, mutable = read(factory)
                self.assertEqual(mutable, 42)
            events.clear()
            result, mutable = read(factory)
            self.assertEqual(events, [()])
            self.assertEqual(mutable, 42)
            reference = weakref.ref(result)
            self.assertIs(reference(), result)
            del result
            gc.collect()
            self.assertIsNone(reference())


if __name__ == "__main__":
    unittest.main()
