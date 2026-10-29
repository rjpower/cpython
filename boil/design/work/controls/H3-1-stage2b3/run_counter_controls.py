"""Task-owned primitive counter controls. prepare is host-only; run needs grant."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[5]
SOURCE = Path(__file__).resolve().parent
E = ROOT / 'build/H3-1-stage2b3/counter-controls'
BUILD = ROOT / 'build/pystats'
NATIVE_HEAD = 'd462e8338ce3b3cee1a74de189345f6665fcc16a'
HISTORICAL = '81417aff0971fe621fec296a324c7a48315a6082'
ENV = {'HOME': '/home/power', 'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'TZ': 'UTC',
       'PYTHONDONTWRITEBYTECODE': '1', 'UV_CACHE_DIR': '/home/power/.cache/uv'}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, data):
    with path.open('x') as output:
        json.dump(data, output, indent=2, sort_keys=True)
        output.write('\n')


def git(*args):
    return subprocess.check_output(['/usr/bin/git', *args], cwd=ROOT, text=True)


def source():
    assert not git('status', '--porcelain'), 'freeze owned source first'
    changes = git('diff', '--name-only', NATIVE_HEAD, 'HEAD').splitlines()
    assert all(name == 'Lib/test/test_pystats.py' or name.startswith(
        'boil/design/work/controls/H3-1-stage2b3/') or name ==
        'boil/design/work/H3-1-stage2b3-implementation.md' for name in changes)
    return git('rev-parse', 'HEAD').strip()


def native_inputs():
    # Include every original build path/alias, not only the executable.
    products_path = ROOT / 'build/H3-1-stage2b3/products-before-cleanup.json'
    products = json.loads(products_path.read_text())
    assert {str(path.relative_to(BUILD)) for path in BUILD.rglob('*')
            if path.is_file()} == set(products), 'native product roster changed'
    receipt = json.loads((ROOT / 'build/H3-1-stage2b3/build-result.json').read_text())
    for tool in receipt['toolchain'].values():
        assert sha(tool['path']) == tool['sha256'], tool['path']
        assert str(Path(tool['path']).resolve()) == tool['resolved'], tool['path']
    for name, row in products.items():
        path = BUILD / name
        assert sha(path) == row['sha256'], name
        assert str(path.resolve()) == row['resolved'], name
        assert (os.readlink(path) if path.is_symlink() else None) == row['symlink'], name
    return {str(path.relative_to(ROOT)): sha(path) for path in (
        products_path, BUILD / 'Makefile', BUILD / 'pyconfig.h', BUILD / 'python',
        ROOT / 'build/H3-1-stage2b3/build-result.json',
        ROOT / 'build/H3-1-stage2b3/post-cleanup.json')}


def macro(text):
    start = text.index('#define INSTRUCTION_STATS(op) \\\n')
    return text[start:text.index('\n#else', start)] + '\n'


def headers():
    current = (ROOT / 'Python/ceval_macros.h').read_text()
    old_macros = git('show', HISTORICAL + ':Python/ceval_macros.h')
    generated = (ROOT / 'Python/generated_ir_cases.c.h').read_text()
    span = re.findall(r' +INSTRUCTION_STATS\(LOAD_CONST\);\n +IR_PREDICTED\(LOAD_CONST\);', generated)
    assert len(span) == 1
    definitions = []
    for expression in (r'^#define IR_PREDICTED\(op\).+$',
                       r'^#  define JUMP_TO_PREDICTED\(name\) goto PREDICTED_##name;'):
        definitions.append(re.search(expression, current, re.M)[0] + '\n')
    old = git('show', HISTORICAL + ':Include/cpython/pystats.h')
    layout = old[old.index('typedef struct'):old.index('// Export for most shared extensions')]
    for name in ('SpecializationStats', 'OpcodeStats', 'CallStats', 'ObjectStats',
                 'GCStats', 'RareEventStats', 'PyStats'):
        layout = re.sub(r'\b' + name + r'\b', 'Old' + name, layout)
    layout = re.sub(r'(struct )(_\w+)', r'\1_old\2', layout)
    moved = '\n'.join(reversed(span[0].splitlines()))
    common = {'old_stats_layout.h': layout, 'predicted_macros.h': ''.join(definitions)}
    return {
        'positive': dict(common, **{'counter_macro.h': macro(current), 'predicted_span.h': span[0] + '\n'}),
        'old_cutoff': dict(common, **{'counter_macro.h': macro(old_macros), 'predicted_span.h': span[0] + '\n'}),
        'counter_after_label': dict(common, **{'counter_macro.h': macro(current), 'predicted_span.h': moved + '\n'}),
    }


def prepare():
    head = source()
    E.mkdir(exist_ok=False)
    inputs = native_inputs()
    originals = ['Python/ceval_macros.h', 'Python/generated_ir_cases.c.h',
                 'Include/cpython/pystats.h', 'Include/internal/pycore_ir_metadata.h',
                 'Include/internal/pycore_stats.h', 'Include/opcode_ids.h',
                 'Python/ceval.c', 'Python/sysmodule.c', 'Python/pystats.c']
    inputs.update({name: sha(ROOT / name) for name in originals})
    inputs.update({str(path.relative_to(ROOT)): sha(path) for path in
                   (Path(__file__), SOURCE / 'stats_counter_fixture.c')})
    copied = {}
    for variant, files in headers().items():
        directory = E / variant
        directory.mkdir()
        for name, content in files.items():
            path = directory / name
            path.write_text(content)
            copied[str(path.relative_to(E))] = sha(path)
    save(E / 'plan.json', {'source_head': head, 'native_source_head': NATIVE_HEAD,
                           'historical_macro_layout_head': HISTORICAL,
                           'inputs': inputs, 'copied_headers': copied,
                           'qualification': False})


def expected(data):
    low, high, nofb = (data[name] for name in ('low_id', 'high_id', 'nofb_id'))
    return {
        'allocation_succeeded': 1, 'allocation_zeroed': 1,
        'pre.low_entries': 2, 'pre.high_entries': 1, 'pre.nofb_entries': 0,
        'pre.initial_pair': 1, 'pre.low_high_pair': 1, 'pre.high_nofb_pair': 0,
        'pre.nofb_low_pair': 1, 'pre.last': low,
        'post.low_entries': 1, 'post.high_entries': 2, 'post.nofb_entries': 1,
        'post.initial_pair': 1, 'post.low_high_pair': 1, 'post.high_nofb_pair': 1,
        'post.nofb_high_pair': 1, 'post.outer_last': high, 'post.inner_last': low,
        'predicted.entries': 1, 'predicted.self_pair': 0,
    }


def run():
    assert sys.argv[1:] == ['run', '--', '-m', 'test']
    plan_path = E / 'plan.json'
    plan = json.loads(plan_path.read_text())
    assert source() == plan['source_head']
    assert not (E / 'result.json').exists()
    for name, digest in plan['inputs'].items():
        assert sha(ROOT / name) == digest, name
    assert native_inputs().items() <= plan['inputs'].items()
    for name, digest in plan['copied_headers'].items():
        assert sha(E / name) == digest, name
    header = (BUILD / 'pyconfig.h').read_text()
    assert '#define Py_STATS 1' in header
    assert '#define _Py_TAIL_CALL_INTERP 1' not in header
    assert '#define Py_DEBUG 1' not in header
    compiler = '/usr/bin/gcc'
    build_receipt = json.loads((ROOT / 'build/H3-1-stage2b3/build-result.json').read_text())
    assert sha(compiler) == build_receipt['toolchain']['gcc']['sha256']
    row = {'plan_sha256': sha(plan_path), 'qualification': False, 'variants': {}}
    for variant in ('positive', 'old_cutoff', 'counter_after_label'):
        directory = E / variant
        output = directory / '_stats_counter_fixture.so'
        command = [compiler, '-shared', '-fPIC', '-O0', '-g', '-UNDEBUG',
                   '-I' + str(BUILD), '-I' + str(ROOT / 'Include'),
                   '-I' + str(ROOT / 'Include/internal'), '-I' + str(directory),
                   str(SOURCE / 'stats_counter_fixture.c'), '-o', str(output)]
        with (directory / 'compile.log').open('x') as log:
            process = subprocess.run(command, cwd=ROOT, env=ENV, stdout=log, stderr=subprocess.STDOUT)
        assert process.returncode == 0, (variant, 'compilation is not a negative oracle')
        probe_command = [str(BUILD / 'python'), '-B', '-c',
                         'import json, sys, _testinternalcapi as c; '
                         'sys.path.insert(0, sys.argv[1]); import _stats_counter_fixture as f; '
                         'print(json.dumps(dict(mode=c.gc_verify_mode(), stress=c.gc_get_stress(), counters=f.probe())))',
                         str(directory)]
        with (directory / 'probe.log').open('x') as log:
            process = subprocess.run(probe_command, cwd=ROOT, env=ENV, stdout=log, stderr=subprocess.STDOUT)
        assert process.returncode == 0, (variant, 'probe must return counters')
        observed = json.loads((directory / 'probe.log').read_text())
        assert observed['mode'] == [0, 0] and observed['stress'] == 0
        data = observed['counters']
        assert data['sizeof_stats'] - data['sizeof_old_stats'] == 230368
        assert data['domain'] == 302 and data['high_id'] > 255 and data['nofb_id'] > 255
        failures = [name for name, count in expected(data).items() if data[name] != count]
        if variant == 'positive':
            assert not failures, failures
        elif variant == 'old_cutoff':
            assert set(failures) == {
                'pre.high_entries', 'pre.low_high_pair', 'pre.nofb_low_pair',
                'post.high_entries', 'post.nofb_entries', 'post.low_high_pair',
                'post.high_nofb_pair', 'post.nofb_high_pair', 'post.outer_last'
            }, failures
        else:
            assert failures == ['predicted.entries', 'predicted.self_pair'], failures
        row['variants'][variant] = {'compile_argv': command, 'probe_argv': probe_command,
            'observed': observed, 'failed_numeric_assertions': failures,
            'extension_sha256': sha(output), 'compile_log_sha256': sha(directory / 'compile.log'),
            'probe_log_sha256': sha(directory / 'probe.log')}
    assert source() == plan['source_head']
    for name, digest in plan['inputs'].items():
        assert sha(ROOT / name) == digest, name
    assert native_inputs().items() <= plan['inputs'].items()
    row['post_command_verified'] = True
    save(E / 'result.json', row)


if __name__ == '__main__':
    if sys.argv[1:] == ['prepare']:
        prepare()
    else:
        run()
