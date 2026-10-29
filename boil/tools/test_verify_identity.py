"""Host controls for v2 identity and targeted result contracts; no native qualification."""

import copy
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


def write_json(path, value):
    path.write_text(json.dumps(value, sort_keys=True))
    return verify.reference(path)


class IdentityTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory(prefix='verify-identity-controls-')
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)
        self.source, self.build, self.tools = [self.root / name for name in ('source', 'build', 'tools')]
        for path in (self.source, self.build, self.tools):
            path.mkdir()
        for name, contents in {'Lib/test/test_control.py': '# control\n',
                               'Lib/test/test_other.py': '# other\n',
                               'Python/control.c': '/* native */\n',
                               'boil/PROCESS.md': 'documentation\n'}.items():
            path = self.source / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(contents)
        for command in (['init', '--quiet'], ['add', '.'],
                        ['-c', 'user.name=Fixture', '-c', 'user.email=fixture@example.invalid',
                         '-c', 'commit.gpgsign=false', 'commit', '--quiet', '-m', 'host fixture']):
            subprocess.run(['git', '-C', str(self.source), *command], check=True, capture_output=True)
        for name in ('python', 'Makefile', 'pyconfig.h', 'config.status', 'pybuilddir.txt',
                     'build/lib/_testinternalcapi.so', 'build/lib/_sysconfigdata_control.py'):
            path = self.build / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(f'host fixture {name}\n')
        for name in ('verify.py', 'verify_launch.py', 'verify_worker.sh'):
            (self.tools / name).write_bytes((Path(verify.__file__).parent / name).read_bytes())
        snapshot = write_json(self.root / 'source.json', verify.source_snapshot(self.source))
        outputs = write_json(self.root / 'outputs.json', verify.build_outputs(self.build))
        self.identity = {'version': 1, 'qualification': 'component-only', 'build': str(self.build),
                         'source_root': str(self.source), 'runtime_root': str(self.source),
                         'tools_root': str(self.tools), 'source_snapshot': snapshot,
                         'runtime_snapshot': snapshot, 'outputs': outputs,
                         'tools': {name: verify.file_record(path) for path in self.tools.iterdir()
                                   for name in (path.name,)}, 'receipt': None}
        log = self.root / 'build.log'
        log.write_text('HOST FIXTURE: no build executed\n')
        self.receipt = {'version': 1, 'status': 'coordinator-qualified', 'build': str(self.build),
                        'source_before': snapshot, 'source_after': snapshot, 'outputs': outputs,
                        'compiler': {'argv': ['host-fixture'], 'executable': verify.reference(log),
                                     'version': verify.reference(log)},
                        'configuration_before': {name: verify.file_record(self.build / name)
                                                 for name in ('Makefile', 'pyconfig.h', 'config.status')},
                        'review': 'synthetic host control', 'commands': []}
        for argv in (['make', '-C', str(self.build), '-s', 'clean'],
                     ['make', '-C', str(self.build), '-s', '-j16'],
                     [str(self.build / 'python'), '-c', 'pass'],
                     [str(self.build / 'python'), *verify.RECEIPT_SMOKE]):
            self.receipt['commands'].append({'argv': argv, 'cwd': str(self.build), 'environment': {},
                                             'started_utc': '2026-10-29T00:00:00Z',
                                             'finished_utc': '2026-10-29T00:01:00Z', 'returncode': 0,
                                             'log': verify.reference(log)})
        self.save_receipt()

    def save_receipt(self):
        self.identity['receipt'] = write_json(self.root / 'receipt.json', self.receipt)

    def check(self):
        ref = write_json(self.root / 'identity.json', self.identity)
        return verify.load_identity(ref, complete=True)

    def test_complete_identity_and_documentation_changes(self):
        self.assertEqual(self.check(), self.identity)
        (self.source / 'boil/PROCESS.md').write_text('concurrent documentation change\n')
        self.assertEqual(self.check(), self.identity)
        subprocess.run(['git', '-C', str(self.source), 'add', 'boil/PROCESS.md'], check=True)
        subprocess.run(['git', '-C', str(self.source), '-c', 'user.name=Fixture',
                        '-c', 'user.email=fixture@example.invalid', '-c', 'commit.gpgsign=false',
                        'commit', '--quiet', '-m', 'documentation only'], check=True)
        self.assertEqual(self.check(), self.identity)
        (self.source / 'Python/control.c').write_text('/* committed executable change */\n')
        subprocess.run(['git', '-C', str(self.source), 'add', 'Python/control.c'], check=True)
        subprocess.run(['git', '-C', str(self.source), '-c', 'user.name=Fixture',
                        '-c', 'user.email=fixture@example.invalid', '-c', 'commit.gpgsign=false',
                        'commit', '--quiet', '-m', 'executable change'], check=True)
        with self.assertRaisesRegex(ValueError, 'snapshot drift'):
            self.check()

    def test_runtime_never_accepts_a_missing_or_unqualified_receipt(self):
        self.identity['receipt'] = None
        with self.assertRaises(TypeError):
            self.check()
        self.receipt['status'] = 'recorded-unreviewed'
        self.save_receipt()
        with self.assertRaisesRegex(ValueError, 'unqualified'):
            self.check()

    def test_source_output_and_tool_drift(self):
        paths = [self.source / 'Python/control.c', self.build / 'build/lib/_testinternalcapi.so',
                 self.tools / 'verify_worker.sh']
        for path in paths:
            with self.subTest(path=path):
                old = path.read_bytes()
                path.write_bytes(old + b'changed')
                with self.assertRaisesRegex(ValueError, 'drift'):
                    self.check()
                path.write_bytes(old)

    def test_build_symlink_binds_target_bytes_and_alias(self):
        alias = self.build / 'build/lib/_testinternalcapi.so'
        alias.unlink()
        target = self.build / 'Modules/_testinternalcapi.so'
        other = self.build / 'Modules/_testinternalcapi.other.so'
        target.parent.mkdir()
        target.write_bytes(b'original extension')
        other.write_bytes(target.read_bytes())
        alias.symlink_to('../../Modules/_testinternalcapi.so')
        self.identity['outputs'] = write_json(self.root / 'outputs.json', verify.build_outputs(self.build))
        self.receipt['outputs'] = self.identity['outputs']
        self.save_receipt()
        self.check()
        target.write_bytes(b'changed extension behind unchanged link')
        with self.assertRaisesRegex(ValueError, 'output drift'):
            self.check()
        target.write_bytes(b'original extension')
        alias.unlink()
        alias.symlink_to('../../Modules/_testinternalcapi.other.so')
        with self.assertRaisesRegex(ValueError, 'output drift'):
            self.check()

    def test_build_symlink_origin_checks_alias_and_target(self):
        alias = self.build / 'build/lib/_testinternalcapi.so'
        alias.unlink()
        target = self.build / 'Modules/_testinternalcapi.so'
        target.parent.mkdir()
        target.write_bytes(b'original extension')
        alias.symlink_to('../../Modules/_testinternalcapi.so')
        self.identity['outputs'] = write_json(self.root / 'outputs.json', verify.build_outputs(self.build))
        actual = launch.origin_record('_testinternalcapi', str(alias), self.identity)
        self.assertEqual(actual['origin'], str(alias))
        self.assertEqual(actual['resolved_origin'], str(target))
        target.write_bytes(b'changed extension behind unchanged link')
        with self.assertRaisesRegex(ValueError, 'hash mismatch'):
            launch.origin_record('_testinternalcapi', str(alias), self.identity)

    def test_external_build_symlink_is_rejected(self):
        alias = self.build / 'build/lib/_testinternalcapi.so'
        alias.unlink()
        target = self.root / '_testinternalcapi.so'
        target.write_bytes(b'external extension')
        alias.symlink_to(target)
        with self.assertRaisesRegex(ValueError, 'external build output'):
            verify.build_outputs(self.build)
        with self.assertRaisesRegex(ValueError, 'external actual build origin'):
            launch.origin_record('_testinternalcapi', str(alias), self.identity)

    def test_narrowed_snapshot_and_untracked_input_fail(self):
        snapshot = verify.read_json(self.root / 'source.json')
        del snapshot['files']['Python/control.c']
        ref = write_json(self.root / 'narrowed.json', snapshot)
        with self.assertRaisesRegex(ValueError, 'snapshot drift'):
            verify.check_snapshot(ref, self.source)
        (self.source / 'Python/untracked.c').write_text('unadmitted input\n')
        with self.assertRaisesRegex(ValueError, 'unadmitted'):
            self.check()

    def test_directory_symlink_identity_does_not_read_a_directory(self):
        link = self.source / 'linked-tests'
        link.symlink_to('Lib/test', target_is_directory=True)
        subprocess.run(['git', '-C', str(self.source), 'add', 'linked-tests'], check=True)
        snapshot = verify.source_snapshot(self.source)
        self.assertEqual(snapshot['files']['linked-tests']['link'], 'Lib/test')
        link.unlink()
        link.symlink_to(self.tools, target_is_directory=True)
        with self.assertRaisesRegex(ValueError, 'external source symlink'):
            verify.source_snapshot(self.source)

    def test_receipt_sequence_returncode_and_binding_fail(self):
        baseline = copy.deepcopy(self.receipt)
        controls = [lambda r: r['commands'][0].update(argv=['true']),
                    lambda r: r['commands'][1].update(returncode=1),
                    lambda r: r.update(build='/wrong/build'),
                    lambda r: r['source_after'].update(sha256='0' * 64),
                    lambda r: r.update(review='')]
        for change in controls:
            self.receipt = copy.deepcopy(baseline)
            change(self.receipt)
            self.save_receipt()
            with self.assertRaises((ValueError, OSError)):
                self.check()

    def test_actual_origin_must_match_name_root_and_hash(self):
        path = self.source / 'Lib/test/test_control.py'
        self.assertEqual(launch.origin_record('test.test_control', str(path), self.identity)['sha256'],
                         verify.digest(path))
        for origin in (str(self.source / 'Lib/test/test_other.py'), str(Path(verify.__file__)), 'frozen'):
            with self.subTest(origin=origin), self.assertRaises(ValueError):
                launch.origin_record('test.test_control', origin, self.identity)
        path.write_text('changed actual module\n')
        with self.assertRaisesRegex(ValueError, 'hash mismatch'):
            launch.origin_record('test.test_control', str(path), self.identity)

    def test_worker_hooks_preserve_calls_and_bind_actual_module_and_skips(self):
        names = ('test', 'test.libregrtest', 'test.libregrtest.single')
        modules = {name: types.ModuleType(name) for name in names}
        single = modules[names[-1]]
        original_run, original_load = Mock(return_value='ran'), Mock(return_value='loaded')
        single.run_unittest, single._load_run_test = original_run, original_load
        single.abs_module_name = lambda name, test_dir: name
        module = types.SimpleNamespace(__name__='test.test_control',
                 __spec__=types.SimpleNamespace(origin=str(self.source / 'Lib/test/test_control.py')))
        result = types.SimpleNamespace(test_name=module.__name__)
        runtests = types.SimpleNamespace(test_dir=None)
        evidence = self.root / 'hooks.jsonl'
        plan = {'evidence': str(evidence)}
        with patch.dict(sys.modules, modules), \
             patch.object(unittest.TestResult, 'addSkip', unittest.TestResult.addSkip), \
             patch.object(launch.importlib.util, 'find_spec', return_value=module.__spec__):
            launch.record_worker_results(plan, {'pid': 2}, self.identity)
            self.assertEqual(single._load_run_test(result, runtests), 'loaded')
            self.assertEqual(single.run_unittest(module, runtests), 'ran')
            test = unittest.FunctionTestCase(lambda: None)
            skipped = unittest.TestResult()
            skipped.addSkip(test, 'host reason')
        original_load.assert_called_once_with(result, runtests)
        original_run.assert_called_once_with(module, runtests)
        rows = [json.loads(line) for line in evidence.read_text().splitlines()]
        self.assertEqual([row['phase'] for row in rows], ['test-module-spec', 'test-module', 'skipped'])
        self.assertEqual(rows[-1]['test_id'], test.id())
        self.assertEqual(rows[-1]['reason'], 'host reason')
        self.assertEqual(skipped.skipped, [(test, 'host reason')])

    def test_runtime_checks_configured_root_cache_and_asan_environment(self):
        (self.build / 'Makefile').write_text(f'abs_srcdir= {self.source}\n')
        self.identity['outputs'] = write_json(self.root / 'runtime-outputs.json', verify.build_outputs(self.build))
        self.identity['tools_root'] = str(Path(launch.__file__).resolve().parent)
        config = {'abs_srcdir': str(self.source), 'Py_DEBUG': 1, 'CONFIG_ARGS': '--with-pydebug'}
        cache = str(self.root / 'fresh-cache')
        plan = {'identity': {}, 'pycache_prefix': cache}
        profile = {'build_kind': 'debug', 'asan_options': None}
        with patch.object(verify, 'load_identity', return_value=self.identity), \
             patch.object(launch.sysconfig, 'get_config_var', side_effect=config.get), \
             patch.object(launch.sysconfig, '_get_sysconfigdata_name', return_value='host-control-config'), \
             patch.object(launch, 'module_origin', return_value={}), \
             patch.object(launch, 'origin_record', return_value={}), \
             patch.object(launch.importlib.util, 'find_spec', return_value=types.SimpleNamespace(origin='host-worker')), \
             patch.object(launch.importlib, 'import_module', return_value=object()) as imported, \
             patch.object(sys, 'executable', str(self.build / 'python')), \
             patch.object(sys, 'pycache_prefix', cache):
            self.assertEqual(launch.runtime_identity(plan, profile, 'driver', 'test')[0], self.identity)
            config['abs_srcdir'] = '/wrong/source'
            with self.assertRaisesRegex(ValueError, 'configured source root'):
                launch.runtime_identity(plan, profile, 'driver', 'test')
            config['abs_srcdir'] = str(self.source)
            with patch.object(sys, 'pycache_prefix', '/wrong/cache'), \
                 self.assertRaisesRegex(ValueError, 'pycache prefix'):
                launch.runtime_identity(plan, profile, 'worker', 'test.libregrtest.worker')
            config['CONFIG_ARGS'] = '--with-pydebug --with-address-sanitizer'
            profile = {'build_kind': 'asan', 'asan_options':
                       {'detect_stack_use_after_return': 0, 'detect_leaks': 0}}
            for actual in ('detect_leaks=1', 'detect_stack_use_after_return=0:detect_leaks=0:detect_leaks=0'):
                with patch.dict(os.environ, ASAN_OPTIONS=actual), \
                     self.assertRaisesRegex(ValueError, 'ASan environment'):
                    launch.runtime_identity(plan, profile, 'worker', 'test.libregrtest.worker')
            with patch.dict(os.environ, ASAN_OPTIONS='detect_stack_use_after_return=0:detect_leaks=0'):
                imported.reset_mock()
                launch.runtime_identity(plan, profile, 'worker', 'test.libregrtest.worker')
                forbidden = {'test.libregrtest.worker', 'test.libregrtest.run_workers', 'test.libregrtest.main'}
                self.assertFalse(forbidden & {call.args[0] for call in imported.call_args_list})
                with patch.dict(sys.modules, {'test.libregrtest.worker': object()}), \
                     self.assertRaisesRegex(ValueError, 'imported before runpy'):
                    launch.runtime_identity(plan, profile, 'worker', 'test.libregrtest.worker')
                imported.reset_mock()
                with patch.dict(sys.modules):
                    sys.modules.pop('platform', None)
                    launch.runtime_identity(plan, profile, 'worker', 'platform')
                self.assertEqual({call.args[0] for call in imported.call_args_list}, {'sysconfig', '_testinternalcapi'})


class ResultTests(unittest.TestCase):
    def setUp(self):
        self.common = {'run': 'host', 'stage': 'main', 'plan_sha256': '0' * 64}
        self.case = 'test.test_control.Case.test_required'
        binding = {'worker_pid': 2, 'tests': ['test.test_control'], 'match_tests': [],
                   'worker_config_sha256': '1' * 64, 'random_seed': 42}
        stats = {'tests_run': 2, 'failures': 0, 'skipped': 0}
        result = {'test_name': 'test.test_control', 'state': 'PASSED', 'stats': stats,
                  'xml_data': [f'<testsuite><testcase name="{self.case}" />'
                               '<testcase name="test.test_control.Case.test_other" /></testsuite>']}
        rows = [dict(pid=1, role='driver', phase='start', mode=[0, 0], stress=0),
                dict(pid=1, phase='selection', tests=['test.test_control']),
                dict(pid=1, phase='scheduled', **binding),
                dict(pid=2, role='worker', phase='start', mode=[0, 0], stress=0, **binding),
                dict(pid=2, phase='test-module-spec', module='test.test_control'),
                dict(pid=2, phase='test-module', module='test.test_control'),
                dict(pid=2, role='worker', phase='end', mode=[0, 0], stress=0, **binding),
                dict(pid=1, phase='returned', result=result, **binding),
                dict(pid=1, phase='summary', stats=stats, state='SUCCESS'),
                dict(pid=1, role='driver', phase='end', mode=[0, 0], stress=0)]
        self.rows = [dict(self.common, **row) for row in rows]
        self.roots = [{'pid': pid, 'interp': 0, 'collections': 1, 'fallbacks': 0} for pid in (1, 2)]
        self.expected = {'test.test_control': {'module': 'test.test_control',
                                             'tests_run': 2, 'allowed_skips': [],
                                             'match_tests': [], 'required_cases': [self.case]}}

    def check(self, rows=None, roots=None):
        return verify.validate_module_results(self.rows if rows is None else rows,
                    self.roots if roots is None else roots, self.expected, {self.case},
                    mode=[0, 0], stress=0, **self.common)

    def test_complete_cohort(self):
        self.assertEqual(self.check()['modules'], 1)

    def test_short_scheduled_name_has_explicit_canonical_origin(self):
        self.expected['test_control'] = self.expected.pop('test.test_control')
        for row in self.rows:
            if 'tests' in row:
                row['tests'] = ['test_control']
            if row['phase'] == 'returned':
                row['result']['test_name'] = 'test_control'
        self.assertEqual(self.check()['modules'], 1)
        self.expected['test_control']['module'] = 'test.substituted'
        with self.assertRaisesRegex(ValueError, 'actual module binding'):
            self.check()

    def test_missing_duplicate_or_substituted_witnesses_fail(self):
        for phase in ('scheduled', 'returned', 'summary', 'selection'):
            with self.subTest(phase=phase), self.assertRaises(ValueError):
                self.check([row for row in self.rows if row['phase'] != phase])
        with self.assertRaisesRegex(ValueError, 'start or end'):
            self.check([row for row in self.rows if not (row['pid'] == 2 and row['phase'] == 'end')])
        for index in (0, 2, 6, 7):
            with self.subTest(duplicate=index), self.assertRaises(ValueError):
                self.check(self.rows + [self.rows[index]])
        for key, value in (('worker_pid', 3), ('worker_config_sha256', '2' * 64), ('random_seed', 43)):
            rows = copy.deepcopy(self.rows)
            rows[7][key] = value
            with self.subTest(binding=key), self.assertRaises(ValueError):
                self.check(rows)

    def test_narrowed_count_protected_skip_and_absent_case_fail(self):
        rows = copy.deepcopy(self.rows)
        rows[7]['result']['stats']['tests_run'] = 1
        with self.assertRaisesRegex(ValueError, 'case count'):
            self.check(rows)
        rows = copy.deepcopy(self.rows)
        rows[7]['result']['xml_data'] = []
        with self.assertRaisesRegex(ValueError, 'concrete case absent'):
            self.check(rows)
        rows = self.rows + [dict(self.common, pid=2, phase='skipped', test_id=self.case, reason='control')]
        with self.assertRaisesRegex(ValueError, 'protected'):
            self.check(rows)
        rows = copy.deepcopy(self.rows)
        rows[7]['result']['xml_data'] = [f'<testsuite><testcase name="{self.case}"><output>xfail</output></testcase></testsuite>']
        with self.assertRaisesRegex(ValueError, 'concrete case absent'):
            self.check(rows)

    def test_missing_counters_fallback_and_zero_collection_fail(self):
        for roots in ([], self.roots[:1], [dict(row, collections=0) for row in self.roots],
                      [dict(row, fallbacks=1) for row in self.roots]):
            with self.subTest(roots=roots), self.assertRaises(ValueError):
                self.check(roots=roots)

    def test_protected_subtest_skips_fail_even_when_allowlisted(self):
        def with_skip(test_id):
            rows = copy.deepcopy(self.rows)
            rows[7]['result']['stats']['skipped'] = 1
            rows[8]['stats']['skipped'] = 1
            rows.insert(6, dict(self.common, pid=2, phase='skipped', test_id=test_id, reason='allowed reason'))
            self.expected['test.test_control']['allowed_skips'] = [(test_id, 'allowed reason')]
            return rows

        # An allowed skip in another method does not hide the required case.
        self.check(with_skip('test.test_control.Case.test_other (params=1)'))
        for test_id in (self.case, self.case + ' (params=1)', self.case + ' [message]'):
            with self.subTest(test_id=test_id), self.assertRaisesRegex(ValueError, 'protected'):
                self.check(with_skip(test_id))


if __name__ == '__main__':
    unittest.main()
