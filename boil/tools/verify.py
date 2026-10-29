#!/usr/bin/env python3
"""Validate and inspect named verification profiles; execution is not implemented."""

import argparse
import collections
import hashlib
import json
import os
import re
import stat
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
DEFAULT_MANIFEST = Path(__file__).with_name('verify.manifest.json')
MODULE = re.compile(r'test(?:\.[A-Za-z_]\w*)+\Z')
GETTERS = {'gc_verify_mode', 'gc_get_stress'}
RECEIPT_SMOKE = ['-m', 'test', '-q', '--timeout', '600', '-j16',
                 '-i', 'test_remote_exec_deleted_static_executable',
                 'test_grammar', 'test_opcodes', 'test_builtin', 'test_dict',
                 'test_list', 'test_tuple', 'test_class', 'test_generators', 'test_capi']


def object_fields(value, required, where):
    if not isinstance(value, dict):
        raise TypeError(f'{where}: expected an object')
    missing = required - value.keys()
    unknown = value.keys() - required
    if missing or unknown:
        raise ValueError(f'{where}: missing {sorted(missing)}, unknown {sorted(unknown)}')
    return value


def nonempty_string(value, where):
    if not isinstance(value, str) or not value.strip():
        raise ValueError(f'{where}: expected a nonempty string')
    return value


def string_list(value, where, *, empty=False):
    if not isinstance(value, list) or (not empty and not value):
        raise ValueError(f'{where}: expected a nonempty list')
    for item in value:
        nonempty_string(item, where)
    if len(set(value)) != len(value):
        raise ValueError(f'{where}: duplicate entries')
    return value


def integer(value, minimum, where):
    if type(value) is not int or value < minimum:
        raise ValueError(f'{where}: expected an integer >= {minimum}')


def module_file(module, repo):
    if not isinstance(module, str) or MODULE.fullmatch(module) is None:
        raise ValueError(f'invalid test module {module!r}')
    path = repo / 'Lib' / module.replace('.', '/')
    for candidate in (path.with_suffix('.py'), path / '__init__.py'):
        if candidate.is_file():
            return candidate
    raise ValueError(f'unknown test module {module!r}')


def unique_keys(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f'duplicate JSON key {key!r}')
        result[key] = value
    return result


def read_json(path):
    with Path(path).open() as stream:
        return json.load(stream, object_pairs_hook=unique_keys)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def absolute_path(value, where):
    if not isinstance(value, str) or not Path(value).is_absolute():
        raise ValueError(f'{where}: expected an absolute path')
    return Path(value).resolve()


def relative_path(value):
    if (not isinstance(value, str) or not value or Path(value).is_absolute()
            or '..' in Path(value).parts or str(Path(value)) != value):
        raise ValueError(f'invalid relative input path: {value!r}')
    return value


def file_record(path):
    path = Path(path)
    link = os.readlink(path) if path.is_symlink() else None
    checksum = hashlib.sha256(os.fsencode(link)).hexdigest() if link is not None else digest(path)
    return {'sha256': checksum, 'mode': stat.S_IMODE(path.lstat().st_mode), 'link': link}


def validate_files(files):
    if not isinstance(files, dict) or not files:
        raise ValueError('expected nonempty file identities')
    for name, record in files.items():
        relative_path(name)
        object_fields(record, {'sha256', 'mode', 'link'}, f'file {name}')
        if (not isinstance(record['sha256'], str)
                or re.fullmatch(r'[0-9a-f]{64}', record['sha256']) is None
                or type(record['mode']) is not int or not 0 <= record['mode'] <= 0o7777
                or record['link'] is not None and not isinstance(record['link'], str)):
            raise ValueError(f'invalid file identity: {name}')


def reference(path):
    return {'path': str(Path(path).resolve()), 'sha256': digest(path)}


def read_reference(ref):
    object_fields(ref, {'path', 'sha256'}, 'file reference')
    path = absolute_path(ref['path'], 'reference.path')
    if (not isinstance(ref['sha256'], str)
            or re.fullmatch(r'[0-9a-f]{64}', ref['sha256']) is None
            or digest(path) != ref['sha256']):
        raise ValueError(f'changed referenced file: {path}')
    return path


def documentation_path(name):
    return (name in {'boil/README.md', 'boil/PROCESS.md', 'boil/WORKLOG.md',
                     'boil/tools/verification.md'}
            or name.startswith('boil/design/') and name.endswith('.md'))


def git_files(root, *options):
    output = subprocess.check_output(['git', '-C', str(root), 'ls-files', '-z', *options])
    return [os.fsdecode(name) for name in output.split(b'\0') if name]


def source_context(identity):
    context = {}
    for name in ('source', 'runtime'):
        root = identity[f'{name}_root']
        recorded = read_json(read_reference(identity[f'{name}_snapshot']))['revision']
        observed = subprocess.check_output(['git', '-C', root, 'rev-parse', 'HEAD'], text=True).strip()
        context[name] = {'recorded_revision': recorded, 'observed_revision': observed}
    return context


def source_snapshot(root, admitted=()):
    root = Path(root).resolve()
    admitted = sorted({relative_path(name) for name in admitted})
    tracked = {name for name in git_files(root) if not documentation_path(name)}
    untracked = git_files(root, '--others', '--exclude-standard')
    input_roots = {'Lib', 'Include', 'Modules', 'Objects', 'Parser', 'Programs',
                   'Python', 'Tools', 'boil'}
    for name in untracked:
        if (Path(name).parts[0] in input_roots and not documentation_path(name)
                and Path(name).suffix in {'.py', '.c', '.h', '.sh'} and name not in admitted):
            raise ValueError(f'unadmitted executable input: {name}')
    paths = tracked | set(admitted)
    for name in paths:
        path = root / name
        if path.is_symlink() and not path.resolve().is_relative_to(root):
            raise ValueError(f'external source symlink: {name}')
    revision = subprocess.check_output(['git', '-C', str(root), 'rev-parse', 'HEAD'], text=True).strip()
    return {'version': 1, 'scope': 1, 'root': str(root), 'revision': revision,
            'admitted': admitted, 'files': {name: file_record(root / name) for name in sorted(paths)}}


def check_snapshot(ref, root, *, complete=True):
    snapshot = read_json(read_reference(ref))
    object_fields(snapshot, {'version', 'scope', 'root', 'revision', 'admitted', 'files'}, 'source snapshot')
    validate_files(snapshot['files'])
    string_list(snapshot['admitted'], 'admitted inputs', empty=True)
    nonempty_string(snapshot['revision'], 'snapshot revision')
    if (type(snapshot['version']) is not int or snapshot['version'] != 1
            or type(snapshot['scope']) is not int or snapshot['scope'] != 1
            or absolute_path(snapshot['root'], 'snapshot.root') != Path(root).resolve()):
        raise ValueError('source snapshot root/version mismatch')
    if complete:
        actual = source_snapshot(root, snapshot['admitted'])
        if any(actual[key] != snapshot[key] for key in snapshot if key != 'revision'):
            raise ValueError(f'source snapshot drift: {root}')
    return snapshot


def build_outputs(build):
    build = Path(build).resolve()
    required = {'python', 'Makefile', 'pyconfig.h', 'config.status', 'pybuilddir.txt'}
    paths = set(required)
    for pattern in ('libpython*.a', 'libpython*.so*', 'build/**/*.so', 'Modules/*.so',
                    'build/**/_sysconfigdata*.py', 'build/**/_sysconfig_vars*.json',
                    'Python/frozen_modules/*.h', 'Python/deepfreeze/*.c', 'Modules/config.c',
                    'Modules/Setup*'):
        paths.update(str(path.relative_to(build)) for path in build.glob(pattern) if path.is_file())
    for name in tuple(paths):
        path = build / name
        if path.is_symlink():
            target = path.resolve(strict=True)
            if not target.is_relative_to(build):
                raise ValueError(f'external build output symlink: {name}')
            paths.add(str(target.relative_to(build)))
    return {'version': 1, 'build': str(build),
            'files': {name: file_record(build / name) for name in sorted(paths)}}


def check_outputs(ref, build, *, complete=True):
    outputs = read_json(read_reference(ref))
    object_fields(outputs, {'version', 'build', 'files'}, 'build outputs')
    validate_files(outputs['files'])
    if not {'python', 'Makefile', 'pyconfig.h', 'config.status', 'pybuilddir.txt'} <= outputs['files'].keys():
        raise ValueError('incomplete build output identities')
    if (type(outputs['version']) is not int or outputs['version'] != 1
            or absolute_path(outputs['build'], 'outputs.build') != Path(build).resolve()):
        raise ValueError('build output root/version mismatch')
    if complete and outputs != build_outputs(build):
        raise ValueError('build output drift')
    return outputs


def validate_receipt(ref, identity):
    receipt = read_json(read_reference(ref))
    object_fields(receipt, {'version', 'status', 'build', 'source_before', 'source_after',
                           'outputs', 'commands', 'compiler', 'configuration_before', 'review'}, 'build receipt')
    if (type(receipt['version']) is not int or receipt['version'] != 1
            or receipt['status'] != 'coordinator-qualified'
            or receipt['build'] != identity['build']
            or receipt['source_before'] != identity['source_snapshot']
            or receipt['source_after']['sha256'] != identity['source_snapshot']['sha256']
            or receipt['outputs'] != identity['outputs']):
        raise ValueError('unqualified or mismatched build receipt')
    read_reference(receipt['source_after'])
    compiler = object_fields(receipt['compiler'], {'argv', 'executable', 'version'}, 'compiler')
    string_list(compiler['argv'], 'compiler argv')
    read_reference(compiler['executable'])
    read_reference(compiler['version'])
    object_fields(receipt['configuration_before'], {'Makefile', 'pyconfig.h', 'config.status'}, 'prior configuration')
    outputs = check_outputs(identity['outputs'], identity['build'], complete=False)
    require(all(value == outputs['files'][name]
                for name, value in receipt['configuration_before'].items()), 'configuration changed during build')
    nonempty_string(receipt['review'], 'receipt.review')
    if not isinstance(receipt['commands'], list) or len(receipt['commands']) != 4:
        raise ValueError('receipt requires clean, build and smoke command records')
    for command in receipt['commands']:
        object_fields(command, {'argv', 'cwd', 'environment', 'started_utc', 'finished_utc',
                                'returncode', 'log'}, 'build command')
        if not isinstance(command['argv'], list) or not command['argv']:
            raise ValueError('missing build argv')
        for arg in command['argv']:
            nonempty_string(arg, 'build argument')
        if (command['cwd'] != identity['build'] or type(command['returncode']) is not int
                or command['returncode'] != 0 or not isinstance(command['environment'], dict)):
            raise ValueError('failed or mismatched build command')
        for key, value in command['environment'].items():
            nonempty_string(key, 'environment variable')
            if not isinstance(value, str):
                raise ValueError('build environment values must be strings')
        for key in ('started_utc', 'finished_utc'):
            nonempty_string(command[key], key)
        read_reference(command['log'])
    build = identity['build']
    if (receipt['commands'][0]['argv'] != ['make', '-C', build, '-s', 'clean']
            or receipt['commands'][1]['argv'] != ['make', '-C', build, '-s', '-j16']
            or receipt['commands'][2]['argv'] != [str(Path(build) / 'python'), '-c', 'pass']
            or receipt['commands'][3]['argv'] != [str(Path(build) / 'python'), *RECEIPT_SMOKE]):
        raise ValueError('receipt does not record the approved clean/build/smoke sequence')
    return receipt


def load_identity(ref, *, complete=False):
    identity = read_json(read_reference(ref))
    object_fields(identity, {'version', 'qualification', 'build', 'source_root', 'runtime_root',
                            'tools_root', 'source_snapshot', 'runtime_snapshot', 'outputs',
                            'tools', 'receipt'}, 'execution identity')
    if type(identity['version']) is not int or identity['version'] != 1:
        raise ValueError('unsupported execution identity')
    for field in ('build', 'source_root', 'runtime_root', 'tools_root'):
        if str(absolute_path(identity[field], field)) != identity[field]:
            raise ValueError(f'{field}: expected canonical path')
    if identity['qualification'] not in ('component-only', 'recorded-build'):
        raise ValueError('unknown identity qualification')
    check_snapshot(identity['source_snapshot'], identity['source_root'], complete=complete)
    check_snapshot(identity['runtime_snapshot'], identity['runtime_root'], complete=complete)
    check_outputs(identity['outputs'], identity['build'], complete=complete)
    validate_files(identity['tools'])
    if not {'verify.py', 'verify_launch.py', 'verify_worker.sh'} <= identity['tools'].keys():
        raise ValueError('missing frozen launcher tools')
    if {path.name for path in Path(identity['tools_root']).iterdir()} != identity['tools'].keys():
        raise ValueError('unlisted frozen tool input')
    for name, expected in identity['tools'].items():
        relative_path(name)
        if file_record(Path(identity['tools_root']) / name) != expected:
            raise ValueError(f'tool drift: {name}')
    validate_receipt(identity['receipt'], identity)
    return identity


def require(condition, message):
    if not condition:
        raise ValueError(message)


def protected_test(test_id, protected):
    return any(test_id == base or test_id.startswith((base + ' (', base + ' ['))
               for base in protected)


def validate_module_results(records, roots, expected, protected, *, run, stage,
                            plan_sha256, mode, stress):
    """Check a qualified targeted cohort; this does not implement a full gate."""
    starts, ends, scheduled, returned = {}, {}, {}, {}
    phases = collections.defaultdict(list)
    skips = collections.defaultdict(list)
    origins = collections.defaultdict(list)
    selections, summaries = [], []
    for row in records:
        require((row['run'], row['stage'], row['plan_sha256']) == (run, stage, plan_sha256),
                'foreign run/stage/plan witness')
        phase = row['phase']
        if phase in ('start', 'end'):
            require(row['mode'] == mode and row['stress'] == stress, 'actual collector state')
            phases[row['pid']].append(phase)
            if phase == 'start':
                require(row['pid'] not in starts, 'duplicate actual process')
                starts[row['pid']] = row
            else:
                ends[row['pid']] = row
        elif phase == 'scheduled':
            pid = row['worker_pid']
            require(pid not in scheduled, 'duplicate scheduled worker')
            scheduled[pid] = row
        elif phase == 'returned':
            name = row['result']['test_name']
            require(name not in returned, 'duplicate returned module')
            returned[name] = row
        elif phase == 'skipped':
            require(not protected_test(row['test_id'], protected), 'skipped protected selector')
            skips[row['pid']].append((row['test_id'], row['reason']))
        elif phase == 'selection':
            selections.append(row)
        elif phase == 'summary':
            summaries.append(row)
        else:
            require(phase in ('test-module', 'test-module-spec'), 'failure/unknown witness phase')
            origins[row['pid']].append((phase, row['module']))
    require(all(value == ['start', 'end'] for value in phases.values()), 'missing/duplicate start or end')
    for pid, start in starts.items():
        for key in ('role', 'tests', 'match_tests', 'worker_config_sha256', 'random_seed'):
            require(start.get(key) == ends[pid].get(key), 'start/end process identity mismatch')
    drivers = {pid for pid, row in starts.items() if row['role'] == 'driver'}
    workers = {pid: row for pid, row in starts.items() if row['role'] == 'worker'}
    require(len(drivers) == 1, 'driver cohort incomplete')
    require(all(row['role'] in ('driver', 'worker', 'auxiliary') for row in starts.values()), 'unknown role')
    require(len(selections) == len(summaries) == 1, 'missing/duplicate selection or summary')
    require(selections[0]['pid'] in drivers and summaries[0]['pid'] in drivers, 'foreign selection/summary')
    require(summaries[0]['state'] == 'SUCCESS', 'unsuccessful driver summary')
    require(set(selections[0]['tests']) == set(expected), 'resolved selection mismatch')
    require(set(workers) == set(scheduled) and len(workers) == len(expected), 'scheduled/actual PID mismatch')
    require(set(returned) == set(expected), 'returned module identity')
    totals = collections.Counter()
    for pid, start in workers.items():
        schedule = scheduled[pid]
        require(len(start['tests']) == 1, 'non-isolated module worker')
        module = start['tests'][0]
        require(module in expected, 'unexpected module')
        wanted = expected[module]
        result = returned[module]
        require(origins[pid] == [('test-module-spec', wanted['module']),
                                 ('test-module', wanted['module'])],
                'missing/substituted actual module binding')
        require(schedule['pid'] in drivers and result['pid'] in drivers, 'foreign scheduler/result')
        require(result['worker_pid'] == pid, 'returned worker PID')
        for key in ('tests', 'match_tests', 'worker_config_sha256', 'random_seed'):
            require(start[key] == schedule[key] == result[key], f'{key} binding mismatch')
        require(start['match_tests'] == wanted['match_tests'], 'module/case narrowing')
        actual = result['result']
        require(actual['state'] == 'PASSED', 'module failed/skipped/changed')
        stats = actual['stats']
        require(isinstance(stats, dict) and set(stats) == {'tests_run', 'failures', 'skipped'},
                'invalid returned test statistics')
        require(all(type(value) is int and value >= 0 for value in stats.values()),
                'invalid returned test counts')
        require(stats is not None and stats['tests_run'] == wanted['tests_run'], 'exact module case count')
        require(stats['failures'] == 0, 'module failure count')
        observed = skips[pid]
        require(len(observed) == len(set(observed)), 'duplicate skip ID/reason')
        require(stats['skipped'] == len(observed), 'missing actual skip witness')
        require(set(observed) <= set(map(tuple, wanted['allowed_skips'])), 'unapproved skip ID/reason')
        cases = {}
        for xml in actual['xml_data'] or ():
            for case in ET.fromstring(xml).iter('testcase'):
                name = case.get('name')
                require(name not in cases, 'duplicate concrete test ID')
                cases[name] = case
        for name in wanted['required_cases']:
            require(name in cases and not any(cases[name].find(tag) is not None
                                             for tag in ('skipped', 'failure', 'error', 'output', 'outcome')),
                    'required concrete case absent/skipped/failed')
        totals.update(stats)
    require(summaries[0]['stats'] == dict(totals), 'summary/result counts mismatch')
    require(roots and sum(row['collections'] for row in roots) > 0, 'missing collection counters')
    require(not sum(row['fallbacks'] for row in roots), 'stack walk fallback')
    require(set(starts) <= {row['pid'] for row in roots if row['interp'] == 0}, 'process missing main-interpreter counters')
    return {'modules': len(returned), 'stats': dict(totals),
            'collections': sum(row['collections'] for row in roots)}


def validate_inventory(inventory, repo):
    object_fields(inventory, {'revision', 'binary', 'cases', 'source_hashes'}, 'inventory')
    nonempty_string(inventory['revision'], 'inventory.revision')
    nonempty_string(inventory['binary'], 'inventory.binary')
    cases = string_list(inventory['cases'], 'inventory.cases')
    hashes = inventory['source_hashes']
    if not isinstance(hashes, dict) or not hashes:
        raise ValueError('inventory.source_hashes: expected nonempty object')
    for relative, expected in hashes.items():
        if not relative.startswith('Lib/test/') or '..' in Path(relative).parts:
            raise ValueError(f'inventory: invalid source path {relative!r}')
        if not isinstance(expected, str) or re.fullmatch(r'[0-9a-f]{64}', expected) is None:
            raise ValueError(f'inventory: invalid source hash for {relative}')
        path = repo / relative
        if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            raise ValueError(f'inventory source drift: {relative}')
    return set(cases)


def validate_registry(data, repo=REPO, inventory=None):
    object_fields(data, {'version', 'provenance', 'prerequisites', 'sets', 'exclusions', 'profiles'}, 'manifest')
    if type(data['version']) is not int or data['version'] != 1:
        raise ValueError(f'unsupported manifest version {data["version"]!r}')
    known_cases = validate_inventory(inventory, repo) if inventory is not None else None
    provenance = object_fields(data['provenance'], {'source', 'discovery_revision', 'discovery_integration_head', 'discovery_log', 'inventory_dependencies', 'historical_reclamation'}, 'provenance')
    for key in ('source', 'discovery_revision', 'discovery_integration_head', 'discovery_log'):
        nonempty_string(provenance[key], f'provenance.{key}')
    dependencies = string_list(provenance['inventory_dependencies'], 'inventory dependencies', empty=True)
    for dependency in dependencies:
        if not dependency.startswith('Lib/test/') or '..' in Path(dependency).parts:
            raise ValueError(f'invalid inventory dependency {dependency!r}')
        if inventory is not None and dependency not in inventory['source_hashes']:
            raise ValueError(f'inventory missing dependency hash: {dependency}')
    history = object_fields(provenance['historical_reclamation'], {'revision', 'runner', 'selection_count', 'module_count', 'note'}, 'historical_reclamation')
    for key in ('revision', 'runner', 'note'):
        nonempty_string(history[key], f'historical_reclamation.{key}')
    for key in ('selection_count', 'module_count'):
        integer(history[key], 1, f'historical_reclamation.{key}')
    for name in ('prerequisites', 'sets', 'exclusions', 'profiles'):
        if not isinstance(data[name], dict) or not data[name]:
            raise ValueError(f'{name}: expected nonempty object')
        for key in data[name]:
            nonempty_string(key, name)
    for name, entry in data['prerequisites'].items():
        where = f'prerequisite {name}'
        object_fields(entry, {'status', 'owner', 'condition', 'evidence', 'runtime_getters'}, where)
        if entry['status'] not in ('available', 'pending'):
            raise ValueError(f'{where}: unknown status')
        for key in ('owner', 'condition'):
            nonempty_string(entry[key], f'{where}.{key}')
        string_list(entry['evidence'], f'{where}.evidence')
        getters = string_list(entry['runtime_getters'], f'{where}.runtime_getters', empty=True)
        if set(getters) - GETTERS:
            raise ValueError(f'{where}: unknown runtime getter')
    for name, entry in data['sets'].items():
        where = f'set {name}'
        object_fields(entry, {'kind', 'items'}, where)
        items = entry['items']
        if not isinstance(items, list) or not items:
            raise ValueError(f'{where}: zero selection')
        if entry['kind'] == 'modules':
            string_list(items, where)
            for module in items:
                module_file(module, repo)
        elif entry['kind'] == 'cases':
            seen = set()
            for item in items:
                object_fields(item, {'module', 'selector'}, where)
                source = module_file(item['module'], repo)
                if inventory is not None and str(source.relative_to(repo)) not in inventory['source_hashes']:
                    raise ValueError(f'{where}: inventory missing module source hash')
                selector = nonempty_string(item['selector'], where)
                if (not selector.startswith(item['module'] + '.')
                        or MODULE.fullmatch(selector) is None
                        or len(selector.split('.')) < len(item['module'].split('.')) + 2):
                    raise ValueError(f'{where}: selector must be a fully qualified test ID')
                if selector in seen:
                    raise ValueError(f'{where}: duplicate selector {selector}')
                seen.add(selector)
                if known_cases is not None and selector not in known_cases:
                    raise ValueError(f'{where}: selector drift / zero matches: {selector}')
        else:
            raise ValueError(f'{where}: unknown selection kind')
    for name, entry in data['exclusions'].items():
        where = f'exclusion {name}'
        object_fields(entry, {'selector', 'status', 'reason', 'owner', 'remove_when', 'evidence', 'profiles'}, where)
        for key in ('selector', 'reason', 'owner', 'remove_when'):
            nonempty_string(entry[key], f'{where}.{key}')
        if entry['status'] not in ('active', 'source-skip', 'retired'):
            raise ValueError(f'{where}: unknown status')
        string_list(entry['evidence'], f'{where}.evidence')
        references = string_list(entry['profiles'], f'{where}.profiles', empty=True)
        if set(references) - data['profiles'].keys():
            raise ValueError(f'{where}: unknown profile')
    fields = {'build_kind', 'gate', 'selection', 'verify', 'extras', 'stress', 'asan_options', 'repeat', 'exclusions', 'requirements', 'root_counters'}
    for name, profile in data['profiles'].items():
        where = f'profile {name}'
        object_fields(profile, fields, where)
        if profile['build_kind'] not in ('release', 'debug', 'asan', 'tailcall'):
            raise ValueError(f'{where}: unknown build kind')
        if type(profile['gate']) is not int or profile['gate'] not in (1, 2, 3):
            raise ValueError(f'{where}: unknown gate')
        if profile['verify'] not in ('off', 'retain', 'kill'):
            raise ValueError(f'{where}: unknown verifier mode')
        extras = string_list(profile['extras'], where + '.extras', empty=True)
        if set(extras) - {'pymem', 'structs'} or (profile['verify'] == 'off' and extras):
            raise ValueError(f'{where}: invalid verifier extras')
        if profile['verify'] == 'kill' and profile['build_kind'] not in ('debug', 'asan'):
            raise ValueError(f'{where}: kill requires a debug build')
        integer(profile['stress'], 0, where + '.stress')
        integer(profile['repeat'], 1, where + '.repeat')
        options = profile['asan_options']
        if profile['build_kind'] == 'asan':
            object_fields(options, {'detect_stack_use_after_return', 'detect_leaks'}, where + '.asan_options')
            if any(type(value) is not int or value != 0 for value in options.values()):
                raise ValueError(f'{where}: unapproved ASan conditions')
        elif options is not None:
            raise ValueError(f'{where}: ASan conditions on a non-ASan profile')
        if profile['root_counters'] not in ('suite', 'stale', 'record'):
            raise ValueError(f'{where}: unknown root-counter policy')
        selection = profile['selection']
        if not isinstance(selection, dict) or 'kind' not in selection:
            raise ValueError(f'{where}: invalid selection')
        kind = selection['kind']
        if kind == 'full':
            object_fields(selection, {'kind'}, where + '.selection')
            if (profile['gate'] not in (2, 3) or profile['repeat'] != 1
                    or profile['root_counters'] != 'suite'):
                raise ValueError(f'{where}: invalid full-suite gate or repeat')
        elif kind in ('modules', 'cases'):
            object_fields(selection, {'kind', 'set'}, where + '.selection')
            set_name = nonempty_string(selection['set'], where + '.selection.set')
            if set_name not in data['sets'] or data['sets'][set_name]['kind'] != kind:
                raise ValueError(f'{where}: unknown or mismatched selection set')
            if profile['gate'] != 1:
                raise ValueError(f'{where}: targeted selection requires gate1')
        elif kind == 'stale':
            object_fields(selection, {'kind', 'attempts'}, where + '.selection')
            integer(selection['attempts'], 1, where + '.attempts')
            if profile['gate'] != 1 or profile['repeat'] != 1 or profile['root_counters'] != 'stale':
                raise ValueError(f'{where}: invalid stale probe conditions')
        else:
            raise ValueError(f'{where}: unknown selection kind')
        if profile['gate'] == 3 and profile['build_kind'] != 'debug':
            raise ValueError(f'{where}: gate3 requires a debug build')
        for field, registry in (('requirements', 'prerequisites'), ('exclusions', 'exclusions')):
            references = string_list(profile[field], where + '.' + field, empty=True)
            if set(references) - data[registry].keys():
                raise ValueError(f'{where}: unknown {field} reference')
        getters = {getter for key in profile['requirements']
                   for getter in data['prerequisites'][key]['runtime_getters']}
        if getters != GETTERS:
            raise ValueError(f'{where}: both actual-state getter contracts are required')
        for exclusion in profile['exclusions']:
            row = data['exclusions'][exclusion]
            if row['status'] != 'active' or name not in row['profiles']:
                raise ValueError(f'{where}: inactive or out-of-scope exclusion {exclusion}')
    for name, entry in data['exclusions'].items():
        actual = {profile for profile, p in data['profiles'].items() if name in p['exclusions']}
        if set(entry['profiles']) != actual:
            raise ValueError(f'exclusion {name}: profile references disagree')
    return data


def show_profile(data, name, manifest_hash, build=None, inventory=None):
    if name not in data['profiles']:
        raise ValueError(f'unknown profile {name!r}')
    profile = data['profiles'][name]
    requirements = {key: data['prerequisites'][key] for key in profile['requirements']}
    blocked = [key for key, row in requirements.items() if row['status'] == 'pending']
    executable = None
    if build is not None:
        executable = Path(build).resolve() / 'python'
        if not executable.is_file() or not os.access(executable, os.X_OK):
            blocked.append(f'missing executable: {executable}')
    selection = profile['selection']
    if selection['kind'] in ('modules', 'cases'):
        selection = dict(selection, items=data['sets'][selection['set']]['items'])
    if selection['kind'] == 'cases' and inventory is None:
        blocked.append('fresh case inventory not supplied')
    mode = {'off': 0, 'retain': 1, 'kill': 2}[profile['verify']]
    extras = sum({'pymem': 1, 'structs': 2}[extra] for extra in profile['extras'])
    return {'profile': name, 'plan_only': True, 'execution': 'not implemented',
            'manifest_sha256': manifest_hash, 'executable': str(executable) if executable else None,
            'conditions': {key: value for key, value in profile.items() if key != 'selection'},
            'selection': selection, 'prerequisites': requirements, 'blocked': blocked,
            'exclusions': {key: data['exclusions'][key] for key in profile['exclusions']},
            'source_skips': {key: row for key, row in data['exclusions'].items()
                             if row['status'] == 'source-skip'},
            'runtime_checks': {'phases': ['start', 'end'], 'gc_verify_mode': [mode, extras],
                               'gc_get_stress': profile['stress'], 'performed': False}}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, default=DEFAULT_MANIFEST)
    sub = parser.add_subparsers(dest='command', required=True)
    validate = sub.add_parser('validate')
    validate.add_argument('--inventory', type=Path)
    show = sub.add_parser('show')
    show.add_argument('profile')
    show.add_argument('--build', type=Path)
    show.add_argument('--inventory', type=Path)
    args = parser.parse_args(argv)
    try:
        inventory = read_json(args.inventory) if args.inventory else None
        data = validate_registry(read_json(args.manifest), inventory=inventory)
        digest = hashlib.sha256(args.manifest.read_bytes()).hexdigest()
        if args.command == 'validate':
            print(json.dumps({'schema_valid': True, 'profiles': len(data['profiles']),
                              'case_inventory_checked': inventory is not None,
                              'execution_performed': False}))
            return 0
        plan = show_profile(data, args.profile, digest, args.build, inventory)
        print(json.dumps(plan, indent=2))
        return 2 if plan['blocked'] else 0
    except (OSError, TypeError, ValueError) as exc:
        parser.exit(2, f'verify: {exc}\n')


if __name__ == '__main__':
    sys.exit(main())
