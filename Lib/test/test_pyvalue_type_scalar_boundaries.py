import asyncio
import unittest


class ScalarBoundaryTests(unittest.TestCase):
    def test_scalar_slot_nomination_and_hash_none(self):
        class BadLength:
            __len__ = 1000
        class BadCall:
            __call__ = 0.0
        class BadGetitem:
            __getitem__ = 2**200
        class BadNew:
            __new__ = 1000
        class NoHash:
            __hash__ = None
        class CallableBase:
            def __call__(self):
                return 'base'
        class ScalarOverride(CallableBase):
            __call__ = 1000

        for obj, operation, expected in (
            (BadLength(), len, "'int' object is not callable"),
            (BadCall(), lambda value: value(), "'float' object is not callable"),
            (BadGetitem(), lambda value: value[0], "'int' object is not callable"),
            (ScalarOverride(), lambda value: value(), "'int' object is not callable"),
        ):
            with self.subTest(type=type(obj).__name__):
                with self.assertRaisesRegex(TypeError, expected):
                    operation(obj)
        with self.assertRaisesRegex(TypeError, "'int' object is not callable"):
            BadNew()
        with self.assertRaisesRegex(TypeError, "'NoneType' object is not callable"):
            type('NoneNew', (), {'__new__': None})()
        with self.assertRaisesRegex(TypeError, "unhashable type: 'NoHash'"):
            hash(NoHash())

    def test_descriptor_get_scalar_success_error_and_root_pinning(self):
        class ScalarGet:
            __get__ = 1000
        class ScalarHost:
            value = ScalarGet()
        with self.assertRaisesRegex(TypeError, "'int' object is not callable"):
            ScalarHost().value

        class PlainDescriptor:
            pass
        plain = PlainDescriptor()
        class PlainHost:
            value = plain
        self.assertIs(PlainHost.value, plain)
        self.assertIs(PlainHost().value, plain)

        events = []
        roots = {}
        result = object()
        class PinnedDescriptor:
            def __get__(self, receiver, owner):
                events.append((self, receiver, owner))
                roots.clear()
                del owner.value
                return result
        pinned = PinnedDescriptor()
        class Owner:
            value = pinned
        instance = Owner()
        roots.update(descriptor=pinned, owner=Owner, instance=instance)
        self.assertIs(instance.value, result)
        self.assertEqual(len(events), 1)
        self.assertIs(events[0][0], pinned)
        self.assertIs(events[0][1], instance)
        self.assertIs(events[0][2], Owner)
        self.assertEqual(roots, {})

        marker = LookupError('descriptor get marker')
        error_reads = []
        class BrokenDescriptor:
            def __get__(self, receiver, owner):
                error_reads.append((receiver, owner))
                raise marker
        class BrokenHost:
            value = BrokenDescriptor()
        with self.assertRaises(LookupError) as caught:
            BrokenHost().value
        self.assertIs(caught.exception, marker)
        self.assertEqual(len(error_reads), 1)

    def test_dynamic_new_lookup_and_constructor_forwarding(self):
        events = []
        class DynamicMeta(type):
            def __getattribute__(cls, name):
                if name == '__new__':
                    events.append(('lookup', cls))
                    return dynamic_new
                return super().__getattribute__(name)
        def dynamic_new(cls, *args, **kwargs):
            events.append(('new', cls, args, kwargs))
            return object.__new__(cls)
        class Dynamic(metaclass=DynamicMeta):
            def __new__(cls, *args, **kwargs):
                return object.__new__(cls)
            def __init__(self, *args, **kwargs):
                events.append(('init', type(self), args, kwargs))
        events.clear()
        value = Dynamic(1, 2, key=3)
        self.assertIs(type(value), Dynamic)
        self.assertEqual([event[0] for event in events], ['lookup', 'new', 'init'])
        self.assertIs(events[0][1], Dynamic)
        self.assertEqual(events[1][2:], ((1, 2), {'key': 3}))
        self.assertEqual(events[2][2:], ((1, 2), {'key': 3}))

        marker = LookupError('dynamic __new__ lookup marker')
        lookups = []
        initializers = []
        class ErrorMeta(type):
            def __getattribute__(cls, name):
                if name == '__new__':
                    lookups.append(cls)
                    raise marker
                return super().__getattribute__(name)
        class Broken(metaclass=ErrorMeta):
            def __new__(cls, *args, **kwargs):
                return object.__new__(cls)
            def __init__(self, *args, **kwargs):
                initializers.append((args, kwargs))
        with self.assertRaises(LookupError) as caught:
            Broken(9, key=10)
        self.assertIs(caught.exception, marker)
        self.assertEqual(lookups, [Broken])
        self.assertEqual(initializers, [])

        static_calls = []
        class StaticNew:
            @staticmethod
            def __new__(cls, *args, **kwargs):
                static_calls.append((cls, args, kwargs))
                return object.__new__(cls)
            def __init__(self, *args, **kwargs):
                pass
        self.assertIs(type(StaticNew('x', option=True)), StaticNew)
        self.assertEqual(static_calls, [(StaticNew, ('x',), {'option': True})])

        bound_calls = []
        class BoundNew:
            @classmethod
            def __new__(cls, *args, **kwargs):
                bound_calls.append((cls, args, kwargs))
                return object.__new__(cls)
            def __init__(self, *args, **kwargs):
                pass
        self.assertIs(type(BoundNew('bound', key=11)), BoundNew)
        self.assertEqual(bound_calls, [(BoundNew, (BoundNew, 'bound'), {'key': 11})])

        class DefaultNew:
            pass
        self.assertIs(type(DefaultNew()), DefaultNew)
        with self.assertRaisesRegex(TypeError, "'int' object is not callable"):
            type('ScalarNew', (), {'__new__': 1000})()
        with self.assertRaisesRegex(TypeError, "'NoneType' object is not callable"):
            type('NoneNewAgain', (), {'__new__': None})()

    def test_context_manager_scalar_exits_and_bound_calls(self):
        sync_events = []
        body_error = ValueError('sync body marker')
        class ScalarExitManager:
            def __enter__(self):
                sync_events.append(('enter', self))
                return self
            __exit__ = 1000
        with self.assertRaisesRegex(TypeError, "'int' object is not callable") as caught:
            with ScalarExitManager():
                raise body_error
        self.assertIs(caught.exception.__context__, body_error)
        self.assertEqual([event[0] for event in sync_events], ['enter'])

        sync_calls = []
        class ExitDescriptor:
            def __get__(self, receiver, owner):
                sync_calls.append(('get', receiver, owner))
                def exit_method(exc_type, exc, traceback):
                    sync_calls.append(('call', exc_type, exc, traceback))
                    return False
                return exit_method
        class DescriptorExitManager:
            def __enter__(self):
                sync_calls.append(('enter', self))
                return self
            __exit__ = ExitDescriptor()
        with DescriptorExitManager():
            pass
        self.assertEqual([event[0] for event in sync_calls], ['get', 'enter', 'call'])
        self.assertIs(sync_calls[0][1], sync_calls[1][1])
        self.assertIs(sync_calls[0][2], DescriptorExitManager)
        self.assertEqual(sync_calls[2][1:], (None, None, None))

        class ScalarEnterManager:
            __enter__ = 1000
            def __exit__(self, *args):
                return False
        with self.assertRaisesRegex(TypeError, "'int' object is not callable"):
            with ScalarEnterManager():
                pass
        class MissingSyncManager:
            pass
        with self.assertRaisesRegex(TypeError, 'context manager protocol'):
            with MissingSyncManager():
                pass

        method_events = []
        class MethodExitManager:
            def __enter__(self):
                return self
            def __exit__(self, exc_type, exc, traceback):
                method_events.append((self, exc_type, exc, traceback))
                return False
        manager = MethodExitManager()
        method_body_error = KeyError('method exit body')
        with self.assertRaises(KeyError) as method_caught:
            with manager:
                raise method_body_error
        self.assertIs(method_caught.exception, method_body_error)
        self.assertEqual(len(method_events), 1)
        self.assertIs(method_events[0][0], manager)
        self.assertIs(method_events[0][1], KeyError)
        self.assertIs(method_events[0][2], method_body_error)
        self.assertIsNotNone(method_events[0][3])

        async_events = []
        async_error = ValueError('async body marker')
        class ScalarAsyncExitManager:
            async def __aenter__(self):
                async_events.append(('enter', self))
                return self
            __aexit__ = 1000
        async def scalar_async_exit_case():
            with self.assertRaisesRegex(TypeError, "'int' object is not callable") as async_caught:
                async with ScalarAsyncExitManager():
                    raise async_error
            self.assertIs(async_caught.exception.__context__, async_error)
            self.assertEqual([event[0] for event in async_events], ['enter'])
        asyncio.run(scalar_async_exit_case())

        async_calls = []
        class AsyncExitDescriptor:
            def __get__(self, receiver, owner):
                async_calls.append(('get', receiver, owner))
                async def exit_method(exc_type, exc, traceback):
                    async_calls.append(('call', exc_type, exc, traceback))
                    return False
                return exit_method
        class DescriptorAsyncExitManager:
            async def __aenter__(self):
                async_calls.append(('enter', self))
                return self
            __aexit__ = AsyncExitDescriptor()
        async def descriptor_async_exit_case():
            async with DescriptorAsyncExitManager():
                pass
        asyncio.run(descriptor_async_exit_case())
        self.assertEqual([event[0] for event in async_calls], ['get', 'enter', 'call'])
        self.assertIs(async_calls[0][1], async_calls[1][1])
        self.assertIs(async_calls[0][2], DescriptorAsyncExitManager)
        self.assertEqual(async_calls[2][1:], (None, None, None))

        class ScalarAsyncEnter:
            __aenter__ = 1000
            async def __aexit__(self, *args):
                return False
        async def scalar_async_enter_case():
            with self.assertRaisesRegex(TypeError, "'int' object is not callable"):
                async with ScalarAsyncEnter():
                    pass
        asyncio.run(scalar_async_enter_case())

        class MissingAsyncManager:
            pass
        async def missing_async_manager_case():
            with self.assertRaisesRegex(TypeError, 'asynchronous context manager'):
                async with MissingAsyncManager():
                    pass
        asyncio.run(missing_async_manager_case())

    def test_metaclass_lookup_precedence_scalars_and_invalid_names(self):
        class DataDescriptor:
            def __get__(self, receiver, owner):
                return ('data', receiver, owner)
            def __set__(self, receiver, value):
                pass
        class NonDataDescriptor:
            def __get__(self, receiver, owner):
                return ('metaclass nondata', receiver, owner)
        class Meta(type):
            data = DataDescriptor()
            nondata = NonDataDescriptor()
            scalar_meta = 2**200
        class Subject(metaclass=Meta):
            data = 'class data'
            nondata = 'class nondata'
            scalar_class = 1000
            static_scalar = staticmethod(0.0)
        data = Subject.data
        self.assertEqual(data[0], 'data')
        self.assertIs(data[1], Subject)
        self.assertIs(data[2], Meta)
        self.assertEqual(Subject.nondata, 'class nondata')
        self.assertEqual(Subject.scalar_class, 1000)
        self.assertIs(type(Subject.scalar_class), int)
        class AttributeName(str):
            pass
        self.assertEqual(type.__getattribute__(Subject, AttributeName('scalar_class')), 1000)
        self.assertEqual(Subject.scalar_meta, 2**200)
        self.assertIs(type(Subject.scalar_meta), int)
        self.assertEqual(Subject.static_scalar, 0.0)
        self.assertIs(type(Subject.static_scalar), float)

        fallback = object()
        self.assertIs(getattr(Subject, 'optional_missing', fallback), fallback)
        with self.assertRaises(AttributeError):
            type.__getattribute__(Subject, 'required_missing')
        with self.assertRaisesRegex(TypeError, "attribute name must be string, not 'int'"):
            type.__getattribute__(Subject, 1000)

        marker = LookupError('metaclass data descriptor marker')
        error_reads = []
        class ErrorDescriptor:
            def __get__(self, receiver, owner):
                error_reads.append((receiver, owner))
                raise marker
        class ErrorMeta(type):
            broken = ErrorDescriptor()
        class BrokenSubject(metaclass=ErrorMeta):
            pass
        with self.assertRaises(LookupError) as caught:
            BrokenSubject.broken
        self.assertIs(caught.exception, marker)
        self.assertEqual(len(error_reads), 1)

        rename_reads = []
        result = object()
        class RenameDescriptor:
            def __get__(self, receiver, owner):
                rename_reads.append((receiver, owner))
                receiver.__name__ = 'RenamedSubject'
                return result
        class RenameMeta(type):
            renamed = RenameDescriptor()
        class RenameSubject(metaclass=RenameMeta):
            pass
        self.assertIs(RenameSubject.renamed, result)
        self.assertEqual(RenameSubject.__name__, 'RenamedSubject')
        self.assertEqual(len(rename_reads), 1)
        self.assertIs(rename_reads[0][0], RenameSubject)
        self.assertIs(rename_reads[0][1], RenameMeta)


if __name__ == '__main__':
    unittest.main()
