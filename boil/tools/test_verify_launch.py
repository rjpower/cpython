"""Safety controls for fixed launch forms and process-state witnesses."""

import copy
import dataclasses
import hashlib
import json
import os
import subprocess
import sys
import tempfile
import types
import unittest
from pathlib import Path
from unittest.mock import Mock, patch

import verify
import verify_launch as launch


class LauncherTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix='verify-launch-controls-')
        self.addCleanup(self.directory.cleanup)
        self.evidence = Path(self.directory.name) / 'evidence.jsonl'
        self.data = verify.read_json(verify.DEFAULT_MANIFEST)
        self.profile = self.data['profiles']['release-full']
        self.plan = {
            'run': 'control', 'profile': 'release-full', 'stage': 'main',
            'manifest_sha256': '0' * 64,
            'launcher_sha256': '0' * 64, 'worker_sha256': '0' * 64,
            'validator_sha256': '0' * 64,
            'evidence': str(self.evidence),
            'jobs': 2, 'selection_index': None,
            'identity': {'path': '/control/identity.json', 'sha256': '0' * 64},
            'xml': str(Path(self.directory.name) / 'results.xml'),
            'pycache_prefix': str(Path(self.directory.name) / 'pycache'),
        }
        self.worker = ['-m', 'test.libregrtest.worker',
                       json.dumps({'tests': ['test_gc'], 'match_tests': [], 'random_seed': 42})]
        self.identity = {'qualification': 'component-only', 'build': self.directory.name,
                         'runtime_root': str(verify.REPO)}
        identity = patch.object(launch, 'runtime_identity', return_value=(self.identity, []))
        self.runtime_identity = identity.start()
        self.addCleanup(identity.stop)
        for target, name in ((launch, 'record_worker_results'), (verify, 'load_identity')):
            mocked = patch.object(target, name, return_value=self.identity)
            mocked.start()
            self.addCleanup(mocked.stop)
        context = patch.object(verify, 'source_context', return_value={})
        context.start()
        self.addCleanup(context.stop)
        self.addCleanup(setattr, sys, 'argv', sys.argv[:])

    def records(self):
        return [json.loads(line) for line in self.evidence.read_text().splitlines()]

    def test_lasting_state_change_fails_after_module(self):
        with patch.object(launch, 'actual_state', side_effect=[((0, 0), 0), ((0, 0), 1000)]), \
             patch.object(launch.runpy, 'run_module') as run, \
             self.assertRaisesRegex(ValueError, 'actual state mismatch at end'):
            launch.execute(self.plan, self.data, self.profile, 'worker', self.worker, '0' * 64)
        run.assert_called_once()
        self.assertEqual([row['phase'] for row in self.records()], ['start', 'end', 'failure'])
        self.assertEqual(self.records()[1]['stress'], 1000)

    def test_worker_error_keeps_nonzero_failure_and_end_witness(self):
        with patch.object(launch, 'actual_state', return_value=((0, 0), 0)), \
             patch.object(launch.runpy, 'run_module', side_effect=ImportError('worker control')), \
             self.assertRaisesRegex(ImportError, 'worker control'):
            launch.execute(self.plan, self.data, self.profile, 'worker', self.worker, '0' * 64)
        self.assertEqual([row['phase'] for row in self.records()], ['start', 'end', 'failure'])
        self.assertTrue(all(row['tests'] == ['test_gc'] for row in self.records()))

    def test_original_error_survives_an_end_state_failure_in_evidence(self):
        with patch.object(launch, 'actual_state', side_effect=[((0, 0), 0), ((0, 0), 1000)]), \
             patch.object(launch.runpy, 'run_module', side_effect=ImportError('original error')), \
             self.assertRaisesRegex(ValueError, 'actual state mismatch at end'):
            launch.execute(self.plan, self.data, self.profile, 'worker', self.worker, '0' * 64)
        self.assertEqual(self.records()[-1]['module_error'], 'ImportError: original error')

    def test_actual_scheduler_result_and_reporter_bindings(self):
        names = ('test', 'test.libregrtest', 'test.libregrtest.run_workers',
                 'test.libregrtest.result', 'test.libregrtest.main')
        modules = {name: types.ModuleType(name) for name in names}
        scheduler, result_module, main = [modules[name] for name in names[2:]]
        original_create = Mock(return_value=types.SimpleNamespace(pid=123))
        scheduler.create_worker_process = original_create

        @dataclasses.dataclass
        class Result:
            test_name: str

        @dataclasses.dataclass
        class Stats:
            tests_run: int
            failures: int
            skipped: int

        result = Result('test.test_control')
        result_module.TestResult = Result
        Result.from_json = staticmethod(lambda value: result)
        reporter = types.SimpleNamespace(run_started=Mock(), summary=Mock())
        started, summary = reporter.run_started, reporter.summary
        main.get_reporter = lambda: reporter
        worker = {'tests': [result.test_name], 'match_tests': [], 'random_seed': 42}
        config = json.dumps(worker)
        runtests = types.SimpleNamespace(as_json=lambda: config, **worker)
        with patch.dict(sys.modules, modules):
            launch.record_scheduling(self.plan, {'pid': 1})
            scheduler.create_worker_process(runtests, 7)
            self.assertIs(Result.from_json('control'), result)
            observed = main.get_reporter()
            observed.run_started(runtests)
            observed.summary(types.SimpleNamespace(stats=Stats(1, 0, 0)), 'SUCCESS')
            with self.assertRaisesRegex(ValueError, 'duplicate'):
                scheduler.create_worker_process(runtests, 7)
        original_create.assert_called_once_with(runtests, 7, None)
        started.assert_called_once_with(runtests)
        summary.assert_called_once()
        schedule, returned, selection, final = self.records()
        self.assertEqual(schedule['worker_config_sha256'], hashlib.sha256(config.encode()).hexdigest())
        for key in ('worker_pid', 'worker_config_sha256', 'random_seed', 'tests', 'match_tests'):
            self.assertEqual(schedule[key], returned[key])
        self.assertEqual(selection['tests'], worker['tests'])
        self.assertEqual(final['stats']['tests_run'], 1)

    def test_platform_is_auxiliary_coverage(self):
        with patch.object(launch, 'actual_state', return_value=((0, 0), 0)), \
             patch.object(launch.runpy, 'run_module') as run:
            launch.execute(self.plan, self.data, self.profile, 'worker', ['-m', 'platform'], '0' * 64)
        run.assert_called_once_with('platform', run_name='__main__', alter_sys=True)
        self.assertEqual([row['phase'] for row in self.records()], ['start', 'end'])
        self.assertTrue(all(row['role'] == 'auxiliary' and not row['tests'] for row in self.records()))

    def test_unknown_auxiliary_and_full_narrowing_never_execute(self):
        controls = [('worker', ['-m', 'test.bisect_cmd']),
                    ('driver', ['-m', 'test', *launch.driver_arguments(self.plan, self.data, self.profile), 'test_gc'])]
        for role, command in controls:
            with self.subTest(role=role), patch.object(launch.runpy, 'run_module') as run:
                with self.assertRaises(ValueError):
                    launch.execute(self.plan, self.data, self.profile, role, command, '0' * 64)
                run.assert_not_called()
        self.assertTrue(all(row['phase'] == 'failure' for row in self.records()))
        self.runtime_identity.assert_not_called()

    def test_ignored_environment_and_missing_getter_fail_before_module(self):
        self.profile = self.data['profiles']['debug-kill-full']
        for state in (((0, 0), 0), AttributeError('gc_get_stress')):
            with self.subTest(state=state), \
                 patch.object(launch, 'actual_state', side_effect=[state]), \
                 patch.object(launch.runpy, 'run_module') as run:
                with self.assertRaises((ValueError, AttributeError)):
                    launch.execute(self.plan, self.data, self.profile, 'worker', self.worker, '0' * 64)
                run.assert_not_called()
        self.assertEqual(self.records()[-1]['phase'], 'failure')

    def test_plan_rejects_changed_identity_and_full_narrowing(self):
        root = Path(self.directory.name)
        binary = root / 'python'
        binary.write_text('control; not executed')
        binary.chmod(0o755)
        plan = dict(self.plan, version=2, manifest=str(verify.DEFAULT_MANIFEST),
                    manifest_sha256=launch.digest(verify.DEFAULT_MANIFEST),
                    build=str(root),
                    inventory=None, inventory_sha256=None,
                    launcher_sha256=launch.digest(launch.__file__),
                    worker_sha256=launch.digest(launch.WORKER),
                    validator_sha256=launch.digest(verify.__file__))
        path = root / 'plan.json'
        path.write_text(json.dumps(plan))
        launch.load_plan(path, launch.digest(path))
        controls = [
            lambda p: p.update(version=1),
            lambda p: p.update(command=['echo never execute']),
            lambda p: p.update(selection_index=0),
            lambda p: p.update(manifest_sha256='0' * 64),
            lambda p: p.update(launcher_sha256='0' * 64),
            lambda p: p.update(stage='cext'),
            lambda p: p.update(jobs=0),
            lambda p: p.update(profile='unknown'),
        ]
        for index, change in enumerate(controls):
            with self.subTest(control=index):
                mutated = copy.deepcopy(plan)
                change(mutated)
                path.write_text(json.dumps(mutated))
                with self.assertRaises(ValueError):
                    launch.load_plan(path, launch.digest(path))

    def test_repeated_module_set_and_fresh_case_scope(self):
        root = Path(self.directory.name)
        binary = root / 'python'
        binary.write_text('control; not executed')
        binary.chmod(0o755)
        data = copy.deepcopy(self.data)
        # This local registry isolates scope validation from the pending
        # ROOTS landing. It is not an executable qualification profile.
        data['prerequisites']['precise-entry-paths']['status'] = 'available'
        manifest = root / 'manifest.json'
        manifest.write_text(json.dumps(data))
        items = data['sets']['reclamation-cases']['items']
        paths = {verify.module_file(item['module'], verify.REPO) for item in items}
        paths.update(verify.REPO / name for name in data['provenance']['inventory_dependencies'])
        inventory = root / 'inventory.json'
        inventory.write_text(json.dumps({
            'revision': 'control', 'binary': 'not-executed',
            'cases': [item['selector'] for item in items],
            'source_hashes': {str(path.relative_to(verify.REPO)): launch.digest(path) for path in paths},
        }))
        plan = dict(self.plan, version=2, manifest=str(manifest), manifest_sha256=launch.digest(manifest),
                    build=str(root),
                    inventory=str(inventory), inventory_sha256=launch.digest(inventory),
                    launcher_sha256=launch.digest(launch.__file__),
                    worker_sha256=launch.digest(launch.WORKER), validator_sha256=launch.digest(verify.__file__))
        path = root / 'plan.json'
        plan['profile'] = 'reclamation20-module-order-debug'
        path.write_text(json.dumps(plan))
        loaded, registry, profile = launch.load_plan(path, launch.digest(path))
        args = launch.driver_arguments(loaded, registry, profile)
        modules = data['sets']['reclamation-modules']['items']
        self.assertEqual(len(modules), 11)
        self.assertEqual(args[4:15], modules)
        self.assertNotIn('-m', args)
        plan['profile'] = 'reclamation20-individual-debug'
        path.write_text(json.dumps(plan))
        with self.assertRaisesRegex(ValueError, 'requires an individual selection index'):
            launch.load_plan(path, launch.digest(path))
        plan['selection_index'] = 0
        path.write_text(json.dumps(plan))
        loaded, registry, profile = launch.load_plan(path, launch.digest(path))
        args = launch.driver_arguments(loaded, registry, profile)
        self.assertEqual(args[4:7], [items[0]['module'], '-m', items[0]['selector']])

    def test_cext_stage_uses_one_isolated_worker_without_filters(self):
        self.plan['stage'] = 'cext'
        self.profile = self.data['profiles']['debug-kill-full']
        args = launch.driver_arguments(self.plan, self.data, self.profile)
        self.assertEqual(args[:5], ['-q', '-u', 'cpu', '-j1', 'test_cext'])
        self.assertEqual(args[5:], ['--junit-xml', self.plan['xml'],
                                   '--python', str(launch.WORKER)])
        self.assertNotIn('-m', args)
        self.assertNotIn('-i', args)

    def test_adapter_preserves_flags_and_rejects_code_options(self):
        binary = Path(self.directory.name) / 'python'
        binary.write_text(f'#!{sys.executable}\nimport json, sys\nprint(json.dumps(sys.argv[1:]))\n')
        binary.chmod(0o755)
        env = dict(os.environ, GATE_VERIFY_PY=str(binary), GATE_VERIFY_PLAN='/control/plan.json')
        adapter = str(launch.WORKER)
        result = subprocess.run([adapter, '-E', '-Werror', '-Xdev', '-P', '-u', '-m', 'platform'],
                                env=env, check=True, capture_output=True, text=True)
        args = json.loads(result.stdout)
        self.assertEqual(args[:5], ['-E', '-Werror', '-Xdev', '-P', '-u'])
        self.assertEqual(args[-2:], ['-m', 'platform'])
        result = subprocess.run([adapter, '-c', 'raise SystemExit(0)'], env=env,
                                check=True, capture_output=True, text=True)
        args = json.loads(result.stdout)
        self.assertIn('--rejected-interpreter-argument', args)
        self.assertEqual(args[-1], '-c')
        self.assertNotIn('raise SystemExit(0)', args)


if __name__ == '__main__':
    unittest.main()
