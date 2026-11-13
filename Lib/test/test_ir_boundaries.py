"""Compact executable offsets and source-only LINE edges."""

import dis
import marshal
import struct
import sys
import unittest
from test.support import import_helper
from test.support.ir_semantics import (observation_records, observation_tables,
                                       semantic_parts, position_parts)

_testinternalcapi = import_helper.import_module('_testinternalcapi')


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
        words = instructions(func)
        self.assertNotIn('NOP', [i.opname for i in words])
        self.assertEqual([i.offset for i in words], list(range(len(words))))
        self.assertEqual(len(list(func.__code__.co_positions())), len(words))
        self.assertTrue(any(start == end for start, end, line in func.__code__.co_lines()))
        restored = marshal.loads(marshal.dumps(func.__code__))
        self.assertEqual(list(restored.co_lines()), list(func.__code__.co_lines()))
        self.assertEqual(list(restored.co_positions()), list(func.__code__.co_positions()))

    def test_empty_raw_location_table_normalizes_real_word_coverage(self):
        def func():
            return 7
        code = func.__code__.replace(co_linetable=b'')
        words = len(instructions(func))
        self.assertTrue(code.co_linetable.startswith(b'IRSM'))
        self.assertEqual(list(code.co_positions()), [(None, None, None, None)] * words)
        self.assertEqual(list(code.co_lines()), [(0, words, None)])
        restored = marshal.loads(marshal.dumps(code))
        self.assertEqual(restored.co_linetable, code.co_linetable)
        self.assertEqual(list(restored.co_positions()), list(code.co_positions()))

    def test_source_map_decoder_bounds_and_stale_replacement(self):
        def func(flag):
            if flag:
                pass
            pass
            return 7
        code = func.__code__
        raw = code.co_linetable
        self.assertEqual(raw[:4], b'IRSM')
        version, locations, records = struct.unpack_from('<III', raw, 4)
        self.assertEqual(version, 2)
        self.assertGreater(records, 0)
        invalid = [b'IRSM', raw[:-1], raw + b'\0',
                   b'\x80\x80', b'\xd0\x80\x00', b'\xd0\x00\x80',
                   raw[:4] + struct.pack('<I', 1) + raw[8:],
                   raw[:8] + struct.pack('<I', locations + 1) + raw[12:]]
        for data in invalid:
            with self.subTest(data=data[:16]):
                with self.assertRaisesRegex(ValueError, 'source map'):
                    code.replace(co_linetable=data)
        semantic = code.co_exceptiontable
        _, _, exception_size, _ = struct.unpack_from('<4sIII', semantic)
        record = 16 + exception_size
        for field, value in ((0, -2), (1, len(code.co_code) // 8),
                             (2, 2), (3, len(code.co_code) // 8)):
            corrupt = bytearray(semantic)
            struct.pack_into('<i', corrupt, record + field * 4, value)
            with self.subTest(field=field):
                with self.assertRaisesRegex(ValueError, 'semantic recovery'):
                    code.replace(co_exceptiontable=bytes(corrupt))
        corrupt = bytearray(raw)
        struct.pack_into('<i', corrupt, 16 + locations, 0)
        with self.assertRaisesRegex(ValueError, 'optional source position'):
            code.replace(co_linetable=bytes(corrupt))
        with self.assertRaisesRegex(ValueError, 'matching semantic recovery'):
            code.replace(co_code=code.co_code[8:] + code.co_code[:8])
        self.assertEqual(code.replace().co_linetable, raw)
        ranges = list(code.co_lines())
        self.assertEqual([start for start, end, line in ranges],
                         sorted(start for start, end, line in ranges))
        self.assertTrue(all(0 <= start <= end <= len(code.co_code) // 8
                            for start, end, line in ranges))

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
        self.assertTrue(any(start == end for start, end, line in ranges))
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

    def test_source_only_lines_and_callback_exception_handler(self):
        caught_lines = []
        def func():
            try:
                pass
                pass
            except LookupError as error:
                caught_lines.append(error.__traceback__.tb_lineno)
                return 'caught'
            raise LookupError('ordinary outside protected region')
        first_pass = func.__code__.co_firstlineno + 2
        for mode in ('trace', 'monitor'):
            with self.subTest(mode=mode):
                seen = []
                caught_lines.clear()
                def line(code, lineno):
                    if code is func.__code__:
                        seen.append(lineno)
                        if lineno == first_pass + 1:
                            raise LookupError('source edge')
                if mode == 'trace':
                    def tracer(frame, event, arg):
                        if event == 'line':
                            line(frame.f_code, frame.f_lineno)
                        return tracer
                    sys.settrace(tracer)
                    try:
                        self.assertEqual(func(), 'caught')
                    finally:
                        sys.settrace(None)
                else:
                    mon = sys.monitoring
                    tool = mon.DEBUGGER_ID
                    mon.use_tool_id(tool, 'source lines')
                    try:
                        mon.register_callback(tool, mon.events.LINE, line)
                        mon.set_local_events(tool, func.__code__, mon.events.LINE)
                        self.assertEqual(func(), 'caught')
                    finally:
                        mon.set_local_events(tool, func.__code__, 0)
                        mon.free_tool_id(tool)
                self.assertIn(first_pass, seen)
                self.assertIn(first_pass + 1, seen)
                self.assertEqual(caught_lines, [first_pass + 1])
                with self.assertRaisesRegex(LookupError, 'ordinary outside'):
                    func()

    def test_nested_source_only_regions_keep_their_handlers(self):
        def func():
            try:
                try:
                    pass
                    pass
                except LookupError:
                    return 'inner'
            except ValueError:
                return 'outer'
            return 'normal'
        last_pass = func.__code__.co_firstlineno + 4
        records = observation_records(func.__code__)
        passes = [record for record in records if record[3] in (last_pass - 1, last_pass)]
        self.assertEqual(len(passes), 2, passes)
        self.assertEqual({record[3] for record in passes}, {last_pass - 1, last_pass})
        self.assertTrue(all(record[7] >= 0 for record in passes), passes)
        for exception, expected in ((LookupError, 'inner'), (ValueError, 'outer')):
            for mode in ('trace', 'monitor'):
                with self.subTest(exception=exception, mode=mode):
                    def line(code, lineno):
                        if code is func.__code__ and lineno == last_pass:
                            raise exception('nested source edge')
                    if mode == 'trace':
                        def tracer(frame, event, arg):
                            if event == 'line':
                                line(frame.f_code, frame.f_lineno)
                            return tracer
                        sys.settrace(tracer)
                        try:
                            self.assertEqual(func(), expected)
                        finally:
                            sys.settrace(None)
                    else:
                        mon = sys.monitoring
                        tool = mon.DEBUGGER_ID
                        mon.use_tool_id(tool, 'nested source lines')
                        try:
                            mon.register_callback(tool, mon.events.LINE, line)
                            mon.set_local_events(tool, func.__code__, mon.events.LINE)
                            self.assertEqual(func(), expected)
                        finally:
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
                records = observation_records(code)
                self.assertTrue(all(words[record[1]].opname != 'RESUME' for record in records))
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

    def test_source_map_cannot_observe_before_generator_resume(self):
        def gen():
            yield 1

        code = gen.__code__
        words = instructions(gen)
        entry = next(i for i in words if i.opname == 'RESUME')
        locations, _ = position_parts(code.co_linetable)
        record = (entry.offset - 1, entry.offset, 0,
                  code.co_firstlineno, code.co_firstlineno, -1, -1, -1, 0, 0)
        exceptions, _ = semantic_parts(code.co_exceptiontable)
        maps = observation_tables(locations, [record], exceptions)
        for handler in ('RESUME', 'INSTRUMENTED_RESUME'):
            with self.subTest(handler=handler):
                encoded = list(struct.unpack(f'<{len(code.co_code) // 8}Q', code.co_code))
                encoded[entry.offset] = ((encoded[entry.offset] & ~65535) |
                                         dis._all_opmap[handler])
                with self.assertRaisesRegex(ValueError, 'semantic recovery.*entry RESUME'):
                    code.replace(co_code=struct.pack(f'<{len(encoded)}Q', *encoded),
                                 **maps)

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
