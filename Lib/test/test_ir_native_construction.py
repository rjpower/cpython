"""Direct generator construction at the shared native owning boundary."""

import dis
import gc
import inspect
import sys
import unittest
import warnings
import weakref

from test import support
from test.support import import_helper, script_helper

_testcapi = import_helper.import_module('_testcapi')


def value_call(factory, value):
    return next(map(factory, (value,)))


def object_call(factory, value):
    return _testcapi.pyobject_vectorcall(factory, (value,), None)


class NativeConstructionTests(unittest.TestCase):
    def test_generator_owners_and_escaped_frame(self):
        def factory(value):
            yield value

        factory(None).close()  # Establish the executable before native entry.
        self.assertEqual(next(dis.get_instructions(factory)).opname, 'RETURN_GENERATOR')

        class Payload:
            pass

        for call in (value_call, object_call):
            with self.subTest(call=call.__name__):
                value = Payload()
                ref = weakref.ref(value)
                generator = call(factory, value)
                self.assertEqual(inspect.getgeneratorstate(generator), inspect.GEN_CREATED)
                frame = generator.gi_frame
                del value
                gc.collect()
                self.assertIsNotNone(ref())
                generator.close()
                self.assertIs(frame.f_locals['value'], ref())
                del generator, frame
                gc.collect()
                self.assertIsNone(ref())

    def test_coroutine_creation_and_return(self):
        async def factory(value):
            return value

        factory(None).close()
        self.assertEqual(next(dis.get_instructions(factory)).opname, 'RETURN_GENERATOR')
        for call in (value_call, object_call):
            with self.subTest(call=call.__name__):
                for value in range(12):
                    coroutine = call(factory, value)
                    self.assertEqual(inspect.getcoroutinestate(coroutine), inspect.CORO_CREATED)
                    with self.assertRaises(StopIteration) as stopped:
                        coroutine.send(None)
                    self.assertEqual(stopped.exception.value, value)

    def test_initial_send_throw_and_close(self):
        def factory(value):
            yield value

        factory(None).close()
        for call in (value_call, object_call):
            with self.subTest(call=call.__name__):
                generator = call(factory, 17)
                with self.assertRaisesRegex(TypeError, 'non-None'):
                    generator.send(23)
                self.assertEqual(next(generator), 17)
                generator.close()
                thrown = call(factory, 23)
                with self.assertRaisesRegex(LookupError, 'initial'):
                    thrown.throw(LookupError('initial'))
                self.assertIsNone(thrown.gi_frame)

    def test_creation_prefix_and_changed_flags_fall_back(self):
        def closure(value):
            def factory(argument):
                yield value, argument
            return factory

        factory = closure(17)
        factory(None).close()
        self.assertNotEqual(next(dis.get_instructions(factory)).opname, 'RETURN_GENERATOR')
        for call in (value_call, object_call):
            self.assertEqual(next(call(factory, 23)), (17, 23))

        def plain(value):
            return value

        plain.__code__ = plain.__code__.replace(
            co_flags=plain.__code__.co_flags | inspect.CO_GENERATOR)
        for call in (value_call, object_call):
            self.assertEqual(call(plain, 31), 31)

    def test_origin_and_keyword_fallback(self):
        async def factory(value):
            return value

        factory(None).close()
        depth = sys.get_coroutine_origin_tracking_depth()
        self.addCleanup(sys.set_coroutine_origin_tracking_depth, depth)
        sys.set_coroutine_origin_tracking_depth(2)
        for call in (value_call, object_call):
            coroutine = call(factory, 17)
            try:
                self.assertIsNotNone(coroutine.cr_origin)
            finally:
                coroutine.close()
        coroutine = _testcapi.pyobject_vectorcall(factory, (23,), ('value',))
        try:
            with self.assertRaises(StopIteration) as stopped:
                coroutine.send(None)
            self.assertEqual(stopped.exception.value, 23)
        finally:
            coroutine.close()

    def test_unawaited_native_coroutine_warns_once(self):
        async def factory(value):
            return value

        factory(None).close()
        with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter('always', RuntimeWarning)
            for call in (value_call, object_call):
                call(factory, 17)
            gc.collect()
        self.assertEqual(len(caught), 2)
        self.assertTrue(all('was never awaited' in str(item.message) for item in caught))

    @support.requires_subprocess()
    def test_allocation_failure_preserves_and_retires_argument_owners(self):
        script_helper.assert_python_ok('-c', '''
import _testcapi
import gc
import weakref

def generator(value):
    yield value

async def coroutine(value):
    return value

class Payload:
    pass

for factory in (generator, coroutine):
    factory(None).close()
    value = Payload()
    ref = weakref.ref(value)
    iterator = map(factory, (value,))
    del value
    _testcapi.set_nomemory(0, 1)
    try:
        try:
            next(iterator)
        except MemoryError:
            pass
        else:
            raise AssertionError('native construction did not fail')
    finally:
        _testcapi.remove_mem_hooks()
    del iterator
    gc.collect()
    assert ref() is None, 'failed construction retained an argument owner'
''')


if __name__ == '__main__':
    unittest.main()
