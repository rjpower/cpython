#!/usr/bin/env python3
"""Validate and inspect named verification profiles; execution is not implemented."""

import argparse
import hashlib
import json
import os
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
DEFAULT_MANIFEST = Path(__file__).with_name('verify.manifest.json')
MODULE = re.compile(r'test(?:\.[A-Za-z_]\w*)+\Z')
GETTERS = {'gc_verify_mode', 'gc_get_stress'}


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
