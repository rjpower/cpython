"""Ownership and artifact boundaries of the reference-counted IR runtime."""
import gc
import importlib.machinery
import importlib.util
import sys
import sysconfig
import unittest
import weakref


class ReferenceCountingRuntimeTests(unittest.TestCase):
    def setUp(self):
        enabled = gc.isenabled()
        gc.disable()
        if enabled:
            self.addCleanup(gc.enable)

    def test_last_reference_runs_finalizer_and_weakref_immediately(self):
        events = []

        class Value:
            def __del__(self):
                events.append('finalizer')

        value = Value()
        reference = weakref.ref(value, lambda ref: events.append('weakref'))
        del value
        self.assertEqual(events, ['finalizer', 'weakref'])
        self.assertIsNone(reference())

    def test_reentrant_replacement_publishes_new_owner_first(self):
        holder = {}
        replacement = object()
        events = []

        class Value:
            def __del__(self):
                events.append(holder['item'] is replacement)
                holder['callback'] = 'completed'

        holder['item'] = Value()
        holder['item'] = replacement
        self.assertEqual(events, [True])
        self.assertEqual(holder['callback'], 'completed')

    def test_owned_container_reference_changes_actual_count(self):
        value = object()
        before = sys.getrefcount(value)
        owner = [value]
        self.assertEqual(sys.getrefcount(value), before + 1)
        del owner
        self.assertEqual(sys.getrefcount(value), before)

    def test_cache_and_extension_names_exclude_other_runtime_abis(self):
        self.assertIn('-valrc1-', sys.implementation.cache_tag)
        self.assertNotIn(importlib.util.MAGIC_NUMBER[:2],
                         (int(3716).to_bytes(2, 'little'),
                          int(3729).to_bytes(2, 'little')))
        if sys.platform.startswith('linux'):
            self.assertTrue(sysconfig.get_config_var('SOABI').startswith('boilvalrc1-'))
            suffixes = importlib.machinery.EXTENSION_SUFFIXES
            self.assertTrue(suffixes)
            self.assertTrue(all(suffix.startswith('.boilvalrc1-') for suffix in suffixes),
                            suffixes)


if __name__ == '__main__':
    unittest.main()
