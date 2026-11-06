import os
from pathlib import Path
import sys
import tempfile
import textwrap
import unittest
from test.support import script_helper

# This function is available for the --enable-pystats config.
HAVE_PYSTATS = hasattr(sys, '_stats_on')

TEST_TEMPLATE = """
    import sys
    import threading
    import time

    THREADS = 2

    class A:
        pass

    class B:
        pass

    def modify_class():
        # This is used as a rare event we can assume doesn't happen unless we do it.
        # It increments the "Rare event (set_class)" count.
        a = A()
        a.__class__ = B

    TURNED_ON = False
    def stats_on():
        global TURNED_ON
        sys._stats_on()
        TURNED_ON = True

    TURNED_OFF = False
    def stats_off():
        global TURNED_OFF
        sys._stats_off()
        TURNED_OFF = True

    CLEARED = False
    def stats_clear():
        global CLEARED
        sys._stats_clear()
        CLEARED = True

    def func_start():
        pass

    def func_end():
        pass

    def func_test(thread_id):
        pass

    _TEST_CODE_

    func_start()
    threads = []
    for i in range(THREADS):
        t = threading.Thread(target=func_test, args=(i,))
        threads.append(t)
        t.start()
    for t in threads:
        t.join()
    func_end()
    """


def run_test_code_stats(
    test_code,
    args=[],
    env_vars=None,
):
    """Run test code and return the stats it dumps, as a dict."""
    code = textwrap.dedent(TEST_TEMPLATE)
    code = code.replace('_TEST_CODE_', textwrap.dedent(test_code))
    script_args = args + ['-c', code]
    env_vars = dict(env_vars or {})
    # Toggle tests control startup collection independently of their parent.
    env_vars.setdefault('PYTHONSTATS', '')
    # Each run dumps into a directory of its own, never the shared default.
    # A process can dump more than once (sys._stats_dump(), then exit), so
    # the counters of all its dumps add up.
    with tempfile.TemporaryDirectory() as dump_dir:
        env_vars.setdefault('PYTHON_PYSTATS_DIR', dump_dir)
        script_helper.run_python_until_end(*script_args, **env_vars)
        stats = {}
        for name in sorted(os.listdir(dump_dir)):
            if not name.endswith('.txt'):
                continue
            with open(os.path.join(dump_dir, name), encoding='ascii',
                      errors='backslashreplace') as f:
                for line in f.read().split('\n'):
                    label, sep, value = line.rpartition(':')
                    value = value.strip()
                    if sep and value.isdigit():
                        label = label.strip()
                        stats[label] = str(int(stats.get(label, 0))
                                           + int(value))
    return stats


def box_kinds(stats):
    return {kind: int(stats[f'Boxing by kind ({kind})'])
            for kind in ('core', 'thunk', 'slot adapter', 'call adapter')}


def run_test_code(
    test_code,
    args=[],
    env_vars=None,
):
    """Run test code and return the value of the "set_class" stats counter.
    """
    stats = run_test_code_stats(test_code, args, env_vars)
    return stats.get('Rare event (set_class)', '')


@unittest.skipUnless(HAVE_PYSTATS, "requires pystats build option")
class TestPyStats(unittest.TestCase):
    """Tests for pystats functionality (requires --enable-pystats build
    option).
    """

    def test_ir_handler_entry_metadata(self):
        code = textwrap.dedent("""
            import _testinternalcapi
            import sys

            def add(a, b):
                return a + b

            assert 'BINARY_OP_ADD' in (
                word[0] for word in _testinternalcapi.get_ir(add.__code__, True))
            sys._stats_on()
            sys._stats_clear()
            assert add(4, 5) == 9
            sys._stats_off()
            sys._stats_dump()
            sys._stats_clear()
        """)
        with tempfile.TemporaryDirectory() as directory:
            script_helper.assert_python_ok('-c', code,
                                           PYTHON_PYSTATS_DIR=directory)
            dumps = list(Path(directory).glob('*.txt'))
            self.assertTrue(dumps)
            for dump in dumps:
                counters = {}
                for line in dump.read_text().splitlines():
                    key, separator, value = line.rpartition(':')
                    if not separator or not value.strip().isdigit():
                        continue
                    key = key.strip()
                    self.assertNotIn(key, counters)
                    counters[key] = int(value)
                self.assertEqual(counters['Handler entry coverage version'], 2)
                self.assertEqual(counters['Handler entry dispatch mode'], 1)
                self.assertGreater(counters['Handler domain count'], 256)
                self.assertGreater(counters['Handler metadata entries'], 256)
                names = {key[len('opcode['):-len('].handler_id')]
                         for key in counters
                         if key.startswith('opcode[')
                         and key.endswith('].handler_id')}
                self.assertEqual(len(names),
                                 counters['Handler metadata entries'])
                ids = {}
                for name in names:
                    prefix = f'opcode[{name}]'
                    identity = counters[prefix + '.handler_id']
                    base = counters[prefix + '.base_id']
                    flags = counters[prefix + '.handler_flags']
                    self.assertLess(identity, counters['Handler domain count'])
                    self.assertNotIn(identity, ids)
                    self.assertTrue(flags & 64)
                    ids[identity] = name
                    self.assertLess(base, counters['Handler domain count'])
                for name in names:
                    base = counters[f'opcode[{name}].base_id']
                    base_name = ids[base]
                    self.assertEqual(counters[f'opcode[{base_name}].base_id'],
                                     base)
                self.assertIn('BINARY_OP_ADD', names)
            self.assertTrue(any(
                'opcode[BINARY_OP_ADD].execution_count' in dump.read_text()
                for dump in dumps))

    def test_malloc_debug_with_stats(self):
        with tempfile.TemporaryDirectory() as directory:
            script_helper.assert_python_ok(
                '-c', 'import test.support',
                PYTHONMALLOC='malloc_debug', PYTHONSTATS='1',
                PYTHON_PYSTATS_DIR=directory)

    def test_owner_metadata_with_failing_python_allocator(self):
        # _testcapi hooks every PyMem domain until process exit. The stats
        # reporter must still resolve the PCs it recorded before the hook.
        code = """
            import atexit
            from test.support import inject_memory_error
            atexit.register(lambda: None)
            inject_memory_error()
        """
        with tempfile.TemporaryDirectory() as directory:
            script_helper.assert_python_ok(
                '-X', 'pystats', '-c', textwrap.dedent(code),
                PYTHONSTATS='', PYTHON_PYSTATS_DIR=directory)
            dumps = list(Path(directory).glob('*.txt'))
            self.assertTrue(dumps)
            for dump in dumps:
                lines = dump.read_text().splitlines()
                owners = [line for line in lines
                          if line.startswith('Box owner (')]
                self.assertTrue(owners)
                self.assertTrue(any(line.startswith('Box object [')
                                    for line in lines))
                self.assertFalse(any('?' in line for line in owners))

    def test_stats_toggle_on(self):
        """Check the toggle on functionality.
        """
        code = """
        def func_start():
            modify_class()
        """

        # If turned on with command line flag, should get one count.
        stat_count = run_test_code(code, args=['-X', 'pystats'])
        self.assertEqual(stat_count, '1')

        # If turned on with env var, should get one count.
        stat_count = run_test_code(code, env_vars={'PYTHONSTATS': '1'})
        self.assertEqual(stat_count, '1')

        # If not turned on, should be no counts.
        stat_count = run_test_code(code)
        self.assertEqual(stat_count, '')

        code = """
        def func_start():
            modify_class()
            sys._stats_on()
            modify_class()
        """
        # Not initially turned on but enabled by sys._stats_on(), should get
        # one count.
        stat_count = run_test_code(code)
        self.assertEqual(stat_count, '1')

    def test_stats_toggle_on_thread(self):
        """Check the toggle on functionality when threads are used.
        """
        code = """
        def func_test(thread_id):
            if thread_id == 0:
                modify_class()
                stats_on()
                modify_class()
            else:
                while not TURNED_ON:
                    pass
                modify_class()
        """
        # Turning on in one thread will count in other thread.
        stat_count = run_test_code(code)
        self.assertEqual(stat_count, '2')

        code = """
        def func_test(thread_id):
            if thread_id == 0:
                modify_class()
                stats_off()
                modify_class()
            else:
                while not TURNED_OFF:
                    pass
                modify_class()
        """
        # Turning off in one thread will not count in other threads.
        stat_count = run_test_code(code, args=['-X', 'pystats'])
        self.assertEqual(stat_count, '1')

    def test_thread_exit_merge(self):
        """Check that per-thread stats (when free-threading enabled) are merged.
        """
        code = """
        def func_test(thread_id):
            modify_class()
            if thread_id == 0:
                raise SystemExit
        """
        # Stats from a thread exiting early should still be counted.
        stat_count = run_test_code(code, args=['-X', 'pystats'])
        self.assertEqual(stat_count, '2')

    def test_stats_dump(self):
        """Check that sys._stats_dump() works.
        """
        code = """
        def func_test(thread_id):
            if thread_id == 0:
                stats_on()
            else:
                while not TURNED_ON:
                    pass
                modify_class()
                sys._stats_dump()
                stats_off()
        """
        # Stats from a thread exiting early should still be counted.
        stat_count = run_test_code(code)
        self.assertEqual(stat_count, '1')

    def test_stats_clear(self):
        """Check that sys._stats_clear() works.
        """
        code = """
        ready = False
        def func_test(thread_id):
            global ready
            if thread_id == 0:
                stats_on()
                modify_class()
                while not ready:
                    pass  # wait until other thread has called modify_class()
                stats_clear()  # clears stats for all threads
            else:
                while not TURNED_ON:
                    pass
                modify_class()
                ready = True
        """
        # Clearing stats will clear for all threads
        stat_count = run_test_code(code)
        self.assertEqual(stat_count, '0')

    def test_box_table(self):
        """Check that the dump has a box count for every site class.
        """
        code = """
        import _testinternalcapi
        def func_start():
            sys._stats_clear()
            items = [1000]
            items.pop()
            _testinternalcapi.pyvalue_contract(1000)
            sys._stats_off()
        """
        stats = run_test_code_stats(code, args=['-X', 'pystats'])
        sites = ['unattributed', 'number', 'compare', 'repr', 'sequence',
                 'iteration', 'attribute', 'call', 'list', 'tuple', 'dict',
                 'set', 'cell/structseq', 'call adapter', 'parser',
                 'interpreter', 'field', 'frame', 'compiler', 'test']
        boxes = {site: int(stats[f'Boxing by site ({site})'])
                 for site in sites}
        self.assertIn('Entry unboxing', stats)
        self.assertEqual(boxes['list'], 0)
        # The test-only owning bridge deliberately projects an inline value.
        self.assertGreaterEqual(boxes['test'], 1)
        self.assertEqual(sum(boxes.values()),
                         int(stats['Object inline int boxes'])
                         + int(stats['Object inline float boxes']))

    def test_box_kinds(self):
        """Check the box count by kind (pyjit/design/proposals/
        value-api-v2a0.md, section 4.7).
        """
        code = """
        import _testcapi
        def func_start():
            sys._stats_clear()
            # The object fastcall requires two adapter projections. The
            # number API deliberately exercises parser and result thunks.
            _testcapi.meth_fastcall(1000, 2000)
            _testcapi.number_add(1000, 2000)
            sys._stats_off()
        """
        stats = run_test_code_stats(code, args=['-X', 'pystats'])
        kinds = box_kinds(stats)
        self.assertGreaterEqual(kinds['call adapter'], 2)
        self.assertGreaterEqual(kinds['thunk'], 1)
        # A call adapter boxes under its own site class, which also holds
        # the THUNK boxes of the parsers' tuple views.
        self.assertLessEqual(kinds['call adapter'],
                             int(stats['Boxing by site (call adapter)']))
        self.assertEqual(sum(kinds.values()),
                         int(stats['Object inline int boxes'])
                         + int(stats['Object inline float boxes']))

    def test_box_kinds_v2a1a(self):
        """The boxes by kind of pyjit/design/proposals/value-api-v2a1a.md,
        section 3, V2a-1a column, with x = 1000. Each statement runs in its
        own process, warmed up first so that the specialised instructions
        are in place.
        """
        # Collection stops right after the statement, so that the template's
        # threads add nothing; the dump at exit still has the counts.
        for statement, expected in (
            # These core paths now stay in value form end to end.
            ("x == 'a'", {'core': 0, 'slot adapter': 0, 'thunk': 0,
                          'call adapter': 0}),
            ("'a' in [x, 2000, 'a']", {'core': 0, 'slot adapter': 0,
                                       'thunk': 0, 'call adapter': 0}),
            ("repr([x, 2000])", {'core': 0, 'slot adapter': 0, 'thunk': 0,
                                 'call adapter': 0}),
            ("operator.add(x, 2000)", {'core': 0, 'slot adapter': 0,
                                       'thunk': 0, 'call adapter': 0}),
        ):
            with self.subTest(statement=statement):
                code = f"""
                import operator
                def func_start():
                    x = 1000
                    for _ in range(100):
                        {statement}
                    sys._stats_clear()
                    {statement}
                    sys._stats_off()
                """
                stats = run_test_code_stats(code, args=['-X', 'pystats'])
                self.assertEqual(box_kinds(stats), expected)


if __name__ == "__main__":
    unittest.main()
