"""Negative controls for verification scope and evidence validation."""

import copy
import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

import verify


class ManifestTests(unittest.TestCase):
    def setUp(self):
        self.data = verify.read_json(verify.DEFAULT_MANIFEST)

    def inventory(self):
        items = self.data['sets']['reclamation-cases']['items']
        paths = {verify.module_file(item['module'], verify.REPO) for item in items}
        paths.update(verify.REPO / name for name in self.data['provenance']['inventory_dependencies'])
        return {'revision': '99435658509', 'binary': 'discovery-only',
                'cases': [item['selector'] for item in items],
                'source_hashes': {str(path.relative_to(verify.REPO)): hashlib.sha256(path.read_bytes()).hexdigest()
                                  for path in paths}}

    def test_registry_and_inventory(self):
        verify.validate_registry(self.data, inventory=self.inventory())
        self.assertEqual(len(self.data['sets']['reclamation-modules']['items']), 11)
        self.assertEqual(len(self.data['sets']['historical-reclamation-modules']['items']), 9)
        self.assertEqual(len(self.data['sets']['reclamation-cases']['items']), 31)

    def test_unknown_and_malformed_fields(self):
        controls = [
            lambda d: d.update(version=2),
            lambda d: d.update(version=True),
            lambda d: d.update(commands=['echo should-never-execute']),
            lambda d: d['profiles']['release-full'].update(verify='kll'),
            lambda d: d['profiles']['release-full'].update(stress=True),
            lambda d: d['profiles']['release-full'].update(root_counters='record'),
            lambda d: d['profiles']['release-full'].update(requirements=[]),
            lambda d: d['profiles']['release-full']['selection'].update(set='stress11'),
            lambda d: d['profiles']['release-full']['selection'].update(matches=['test_gc']),
            lambda d: d['profiles']['stress11-release']['selection'].update(set=[]),
            lambda d: d['sets']['stress11'].update(items=[]),
            lambda d: d['sets']['stress11'].update(items=['test.test_registry_typo']),
            lambda d: d['profiles']['release-full'].update(exclusions=['unknown']),
            lambda d: d['profiles']['asan14-kill'].update(asan_options={'detect_leaks': 0}),
        ]
        for index, mutate in enumerate(controls):
            with self.subTest(control=index):
                data = copy.deepcopy(self.data)
                mutate(data)
                with self.assertRaises(ValueError):
                    verify.validate_registry(data)

    def test_selector_drift_is_not_zero_tests_success(self):
        data = copy.deepcopy(self.data)
        data['sets']['reclamation-cases']['items'][0]['selector'] += '_typo'
        with self.assertRaisesRegex(ValueError, 'selector drift / zero matches'):
            verify.validate_registry(data, inventory=self.inventory())

    def test_missing_and_stale_inventory_evidence(self):
        for field in ('cases', 'source_hashes'):
            with self.subTest(field=field):
                inventory = self.inventory()
                inventory.pop(field)
                with self.assertRaises(ValueError):
                    verify.validate_registry(self.data, inventory=inventory)
        inventory = self.inventory()
        inventory['source_hashes']['Lib/test/test_gc.py'] = '0' * 64
        with self.assertRaisesRegex(ValueError, 'source drift'):
            verify.validate_registry(self.data, inventory=inventory)
        inventory = self.inventory()
        inventory['source_hashes'].pop('Lib/test/_test_multiprocessing.py')
        with self.assertRaisesRegex(ValueError, 'missing dependency hash'):
            verify.validate_registry(self.data, inventory=inventory)

    def test_unavailable_show_never_claims_runtime_checks(self):
        self.data['prerequisites']['stress-state']['status'] = 'pending'
        plan = verify.show_profile(self.data, 'stress11-debug', 'hash')
        self.assertIn('stress-state', plan['blocked'])
        self.assertFalse(plan['runtime_checks']['performed'])
        self.assertEqual(plan['execution'], 'not implemented')
        with self.assertRaisesRegex(ValueError, 'unknown profile'):
            verify.show_profile(self.data, 'typo', 'hash')

    def test_duplicate_json_keys(self):
        with tempfile.TemporaryDirectory(prefix='verify-controls-') as directory:
            path = Path(directory) / 'duplicate.json'
            path.write_text('{"version": 1, "version": 1}')
            with self.assertRaisesRegex(ValueError, 'duplicate JSON key'):
                verify.read_json(path)

    def test_cli_rejects_narrowing_and_unavailable_profiles(self):
        tool = Path(verify.__file__)
        commands = [
            ['show', 'unknown'],
            ['show', 'stress11-debug'],
            ['show', 'release-full', 'test_gc'],
            ['show', 'release-full', '--python', 'echo unexpected'],
            ['run', 'release-full'],
        ]
        # Make the unavailable-profile control independent of landing progress.
        data = copy.deepcopy(self.data)
        data['prerequisites']['stress-state']['status'] = 'pending'
        with tempfile.TemporaryDirectory(prefix='verify-cli-controls-') as directory:
            manifest = Path(directory) / 'manifest.json'
            manifest.write_text(json.dumps(data))
            for args in commands:
                with self.subTest(args=args):
                    result = subprocess.run(
                        [sys.executable, str(tool), '--manifest', str(manifest), *args],
                        check=False, capture_output=True, text=True)
                    self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                    self.assertNotIn('Traceback', result.stderr)


if __name__ == '__main__':
    unittest.main()
