"""Compact executable offsets and source-only LINE edges."""

import dis
import marshal
import sys
import unittest
from test.support import import_helper
from test.support.ir_dis import IRCode

_testinternalcapi = import_helper.import_module('_testinternalcapi')


# Instruction events and offsets name the stack bytecode
def instructions(func):
    return list(dis.get_instructions(func))


def warm(func, *args):
    for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 10):
        func(*args)


class CompactBoundaryTests(unittest.TestCase):
    def test_branch_guards_have_no_empty_words(self):
        def func(flag):
            if flag:
                pass
                return 11
            pass
            return 22
        warm(func, False)
        for flag, expected in ((False, 22), (True, 11), ([], 22), ([1], 11)):
            self.assertEqual(func(flag), expected)
        # The stack bytecode keeps its NOPs; the IR it lowers to has none
        # and one position per word
        ir = IRCode(func.__code__)
        names = [row[0] for row in _testinternalcapi.get_ir(func.__code__)]
        self.assertFalse([name for name in names if name.split('+')[0] == 'NOP'])
        self.assertEqual(len(list(ir.co_positions())), len(names))
        restored = marshal.loads(marshal.dumps(func.__code__))
        self.assertEqual(list(restored.co_lines()), list(func.__code__.co_lines()))
        self.assertEqual(list(restored.co_positions()), list(func.__code__.co_positions()))

    def test_line_ranges_coalesce_adjacent_equal_source_lines(self):
        def func(flag):
            if flag:
                pass
            else:
                pass
            return 7
        positions = list(func.__code__.co_positions())
        ranges = list(func.__code__.co_lines())
        for previous, current in zip(ranges, ranges[1:]):
            self.assertNotEqual(previous[2], current[2], (previous, current))
        self.assertEqual(list(func.__code__.co_positions()), positions)

    def test_late_instruction_monitoring_names_only_real_words(self):
        def func(flag, enable):
            enable()
            if flag:
                pass
                return 1
            pass
            return 2
        warm(func, False, lambda: None)
        mon = sys.monitoring
        tool = mon.DEBUGGER_ID
        real = {i.offset for i in instructions(func)}
        for local in (False, True):
            with self.subTest(local=local):
                seen = []
                mon.use_tool_id(tool, 'compact instructions')
                try:
                    mon.register_callback(tool, mon.events.INSTRUCTION,
                        lambda code, offset: seen.append(offset) if code is func.__code__ else None)
                    def enable():
                        if local:
                            mon.set_local_events(tool, func.__code__, mon.events.INSTRUCTION)
                        else:
                            mon.set_events(tool, mon.events.INSTRUCTION)
                    self.assertEqual(func(False, enable), 2)
                    self.assertTrue(seen)
                    self.assertLessEqual(set(seen), real)
                finally:
                    mon.set_events(tool, 0)
                    mon.set_local_events(tool, func.__code__, 0)
                    mon.free_tool_id(tool)

    def test_empty_branch_join_and_loop_backedge_lines(self):
        def branch(flag):
            if flag:
                pass
            else:
                pass
            pass
            return 7
        def loop():
            for _ in range(3):
                pass
            return 9
        for mode in ('trace', 'monitor'):
            for flag in (False, True):
                with self.subTest(mode=mode, flag=flag):
                    seen = []
                    codes = {branch.__code__, loop.__code__}
                    def line(code, lineno):
                        if code in codes:
                            seen.append((code, lineno))
                    if mode == 'trace':
                        def tracer(frame, event, arg):
                            if event == 'line':
                                line(frame.f_code, frame.f_lineno)
                            return tracer
                        sys.settrace(tracer)
                    else:
                        mon = sys.monitoring
                        tool = mon.DEBUGGER_ID
                        mon.use_tool_id(tool, 'empty edges')
                        mon.register_callback(tool, mon.events.LINE, line)
                        for code in codes:
                            mon.set_local_events(tool, code, mon.events.LINE)
                    try:
                        self.assertEqual(branch(flag), 7)
                        self.assertEqual(loop(), 9)
                    finally:
                        if mode == 'trace':
                            sys.settrace(None)
                        else:
                            for code in codes:
                                mon.set_local_events(tool, code, 0)
                            mon.free_tool_id(tool)
                    selected = branch.__code__.co_firstlineno + (2 if flag else 4)
                    unselected = branch.__code__.co_firstlineno + (4 if flag else 2)
                    self.assertIn((branch.__code__, selected), seen)
                    self.assertNotIn((branch.__code__, unselected), seen)
                    self.assertIn((branch.__code__, branch.__code__.co_firstlineno + 5), seen)
                    self.assertEqual(seen.count((loop.__code__, loop.__code__.co_firstlineno + 2)), 3)

    def test_suspended_generator_has_real_instruction_events_and_source_lines(self):
        def gen(flag):
            yield 0
            if flag:
                pass
                yield 1
            else:
                pass
                yield 2
        warm(lambda: list(gen(False)))
        it = gen(False)
        self.assertEqual(next(it), 0)
        seen = []
        mon = sys.monitoring
        tool = mon.DEBUGGER_ID
        mon.use_tool_id(tool, 'suspended compact stream')
        try:
            mon.register_callback(tool, mon.events.INSTRUCTION,
                lambda code, offset: seen.append(offset) if code is gen.__code__ else None)
            mon.set_local_events(tool, gen.__code__, mon.events.INSTRUCTION)
            self.assertEqual(next(it), 2)
            self.assertTrue(seen)
            self.assertLessEqual(set(seen), {i.offset for i in instructions(gen)})
        finally:
            mon.set_local_events(tool, gen.__code__, 0)
            mon.free_tool_id(tool)
            it.close()

    def test_generator_entry_events_precede_source_lines_after_reconfiguration(self):
        mon = sys.monitoring
        tool = mon.DEBUGGER_ID
        events = mon.events
        mon.use_tool_id(tool, 'generator-entry-source-order')
        self.addCleanup(mon.free_tool_id, tool)
        for event in (events.PY_START, events.PY_RESUME, events.LINE):
            self.addCleanup(mon.register_callback, tool, event, None)
        for warmed in (False, True):
            with self.subTest(warmed=warmed):
                def gen():
                    pass
                    yield 1
                    pass
                    yield 2

                gen.__code__ = gen.__code__.replace()
                if warmed:
                    warm(lambda: list(gen()))
                code = gen.__code__
                words = {i.offset: i for i in instructions(gen)}
                seen = []

                def on_start(observed, offset):
                    if observed is code:
                        self.assertEqual(words[offset].opname, 'RESUME')
                        seen.append(('start',))

                def on_resume(observed, offset):
                    if observed is code:
                        self.assertEqual(words[offset].opname, 'RESUME')
                        seen.append(('resume',))

                def on_line(observed, line):
                    if observed is code:
                        seen.append(('line', line - code.co_firstlineno))

                mon.register_callback(tool, events.PY_START, on_start)
                mon.register_callback(tool, events.PY_RESUME, on_resume)
                mon.register_callback(tool, events.LINE, on_line)
                iterator = gen()
                try:
                    mon.set_local_events(tool, code, events.PY_START | events.PY_RESUME | events.LINE)
                    # The suspended code is absent from the executing-frame
                    # walk; its own RESUME must refresh before any observer.
                    mon.set_events(tool, events.PY_START)
                    self.assertEqual(next(iterator), 1)
                    self.assertEqual(seen, [('start',), ('line', 1), ('line', 2)])
                    mon.restart_events()
                    self.assertEqual(next(iterator), 2)
                    self.assertEqual(seen, [('start',), ('line', 1), ('line', 2),
                                            ('resume',), ('line', 3), ('line', 4)])
                finally:
                    mon.set_events(tool, 0)
                    mon.set_local_events(tool, code, 0)
                    iterator.close()

    def test_observed_delegation_dispatches_real_handlers(self):
        def leaf():
            yield 1
            yield 2
        def delegate(iterable):
            yield from iterable
        for mode in ('trace', 'monitor'):
            for source in (range(2), leaf()):
                with self.subTest(mode=mode, source=type(source)):
                    events = []
                    if mode == 'trace':
                        def tracer(frame, event, arg):
                            if frame.f_code is delegate.__code__ and event == 'line':
                                events.append(frame.f_lineno)
                            return tracer
                        sys.settrace(tracer)
                    else:
                        mon = sys.monitoring
                        tool = mon.DEBUGGER_ID
                        mon.use_tool_id(tool, 'observed delegation')
                        mon.register_callback(tool, mon.events.LINE,
                            lambda code, line: events.append(line) if code is delegate.__code__ else None)
                        mon.set_local_events(tool, delegate.__code__, mon.events.LINE)
                    try:
                        expected = [0, 1] if isinstance(source, range) else [1, 2]
                        self.assertEqual(list(delegate(source)), expected)
                        self.assertTrue(events)
                    finally:
                        if mode == 'trace':
                            sys.settrace(None)
                        else:
                            mon.set_local_events(tool, delegate.__code__, 0)
                            mon.free_tool_id(tool)

    def test_line_jump_preserves_locals_without_missing_temporary(self):
        def func():
            value = ['owner']
            pass
            value.append('skipped')
            return value
        jumped = False
        def tracer(frame, event, arg):
            nonlocal jumped
            if frame.f_code is func.__code__ and event == 'line' and not jumped:
                if frame.f_lineno == func.__code__.co_firstlineno + 2:
                    frame.f_lineno = func.__code__.co_firstlineno + 4
                    jumped = True
            return tracer
        sys.settrace(tracer)
        try:
            self.assertEqual(func(), ['owner'])
        finally:
            sys.settrace(None)
        self.assertTrue(jumped)


if __name__ == '__main__':
    unittest.main()
