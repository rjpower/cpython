import json
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


def run_test_code(
    test_code,
    args=[],
    env_vars=None,
):
    """Run test code and return the value of the "set_class" stats counter.
    """
    code = textwrap.dedent(TEST_TEMPLATE)
    code = code.replace('_TEST_CODE_', textwrap.dedent(test_code))
    script_args = args + ['-c', code]
    with tempfile.TemporaryDirectory() as directory:
        env_vars = dict(env_vars or {}, PYTHON_STATS_DIR=directory)
        script_helper.assert_python_ok(*script_args, **env_vars)
        values = set()
        for path in Path(directory).glob('*.txt'):
            for line in path.read_text().splitlines():
                if line.startswith('Rare event (set_class):'):
                    values.add(line.partition(':')[2].strip())
        # An explicit dump and the final dump can contain the same snapshot.
        # Neither depends on whether the shared default directory exists.
        if not values:
            return ''
        if len(values) != 1:
            raise AssertionError(f'inconsistent set_class snapshots: {values}')
        return values.pop()


@unittest.skipUnless(HAVE_PYSTATS, "requires pystats build option")
class TestPyStats(unittest.TestCase):
    """Tests for pystats functionality (requires --enable-pystats build
    option).
    """

    def test_raw_ir_handler_entries(self):
        # Use legitimate compiled per-operation, shared-feedback and relaxed
        # loop targets, and witness the executing copy before counting them.
        code = textwrap.dedent("""
            import json, sys
            import _opcode_metadata as metadata
            import _testinternalcapi as capi

            def add(a, b):
                return a + b

            namespace = {}
            previous = capi.set_ir_max_forward(0)
            try:
                exec('def jump(it):\\n    for value in it:\\n        pass\\n    return 3\\n', namespace)
            finally:
                capi.set_ir_max_forward(previous)
            jump = namespace['jump']
            # IR_FIELD_MAX sites have private feedback; later sites use NOFB.
            exec('def attrs(obj):\\n' + '    value = obj.attr\\n' * (4095 + 2)
                 + '    return value\\n', namespace)
            attrs = namespace['attrs']
            class Owner:
                attr = 5
            owner = Owner()
            assert add(4, 5) == 9
            assert jump(()) == 3
            assert attrs(owner) == 5
            targets = ((add, 'BINARY_OP_ADD'), (jump, 'IR_JUMP'),
                       (attrs, 'LOAD_ATTR_NOFB'))
            witnessed = {}
            for function, name in targets:
                assert name in [word[0] for word in capi.get_ir(function.__code__, True)]
                witnessed[name] = metadata._ir_opmap[name]
            sys._stats_on()
            sys._stats_clear()
            assert add(4, 5) == 9
            assert jump(()) == 3
            assert attrs(owner) == 5
            sys._stats_off()
            sys._stats_dump()
            sys._stats_clear()
            print(json.dumps(witnessed))
        """)
        with tempfile.TemporaryDirectory() as directory:
            _, stdout, _ = script_helper.assert_python_ok(
                '-c', code, PYTHON_STATS_DIR=directory)
            files = list(Path(directory).glob('*.txt'))
            self.assertEqual(len(files), 1)
            counters = {}
            for line in files[0].read_text().splitlines():
                key, value = line.rsplit(':', 1)
                self.assertNotIn(key.strip(), counters)
                counters[key.strip()] = int(value)
        self.assertEqual(counters['Handler entry coverage version'], 2)
        self.assertIn(counters['Handler entry dispatch mode'], (0, 1))
        for name, identity in json.loads(stdout).items():
            with self.subTest(target=name):
                self.assertEqual(counters[f'opcode[{name}].handler_id'], identity)
                self.assertGreater(counters[f'opcode[{name}].execution_count'], 0)
                self.assertTrue(any(key.endswith(f'].pair_count[{name}]')
                                    and value > 0 for key, value in counters.items()))

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
        """Stats remain in the interpreter aggregate after a thread exits.
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


if __name__ == "__main__":
    unittest.main()
