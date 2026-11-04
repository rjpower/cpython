import sys
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
    env_vars = env_vars or {}
    res, _ = script_helper.run_python_until_end(*script_args, **env_vars)
    stderr = res.err.decode("ascii", "backslashreplace")
    stats = {}
    for line in stderr.split('\n'):
        label, sep, value = line.rpartition(':')
        if sep:
            stats[label.strip()] = value.strip()
    return stats


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
        def func_start():
            items = [1000]
            items.pop()
        """
        stats = run_test_code_stats(code, args=['-X', 'pystats'])
        sites = ['unattributed', 'number', 'compare', 'repr', 'sequence',
                 'iteration', 'attribute', 'call', 'list', 'tuple', 'dict',
                 'set', 'cell/structseq', 'call adapter', 'parser',
                 'interpreter', 'field', 'frame', 'compiler']
        boxes = {site: int(stats[f'Boxing by site ({site})'])
                 for site in sites}
        for site in sites:
            self.assertIn(f'Entry unboxing by site ({site})', stats)
        # list.pop() boxes the inline int it removes.
        self.assertGreaterEqual(boxes['list'], 1)
        self.assertEqual(sum(boxes.values()),
                         int(stats['Object inline int boxes'])
                         + int(stats['Object inline float boxes']))


if __name__ == "__main__":
    unittest.main()
