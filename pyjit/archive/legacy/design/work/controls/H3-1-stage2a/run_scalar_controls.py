"""Compile task-owned header variants; never change live source or binaries."""
from pathlib import Path
import difflib
import hashlib
import json
from opcode import opmap
import shlex
import subprocess
import sys
import sysconfig

repo = next(parent for parent in Path(__file__).resolve().parents
            if (parent / 'Include/internal/pycore_ir.h').is_file())
out = repo / 'build/H3-1-stage2a' / ('scalar-controls-' + Path(sys.executable).resolve().parent.name)
out.mkdir(exist_ok=True)
header_path = repo / 'Include/internal/pycore_ir.h'
header = header_path.read_text()
capi = (repo / 'Modules/_testinternalcapi.c').read_text()
start = capi.index('static PyObject *\nir_scalar_boundary(')
end = capi.index('\nstatic PyMethodDef module_functions', start)
fixture = capi[start:end]
source = '#define Py_BUILD_CORE_MODULE\n#include "Python.h"\n#include "pycore_ir.h"\n' + fixture + '''
static PyMethodDef methods[] = {
    {"ir_scalar_boundary", ir_scalar_boundary, METH_VARARGS},
    {NULL, NULL, 0, NULL}
};
static struct PyModuleDef definition = {
    PyModuleDef_HEAD_INIT, "_h3_scalar_control", NULL, -1, methods
};
PyMODINIT_FUNC PyInit__h3_scalar_control(void) {
    return PyModule_Create(&definition);
}
'''
variants = {'positive': header}
for name, prop in [('make', 'PRISTINE'), ('rewrite', 'DISPATCH')]:
    original = f'if (!_PyIR_HasProperty(handler, _PyIR_H_{prop}))'
    assert header.count(original) == 1
    # Model a cast at the actual producer boundary before its validation.
    variants[name] = header.replace(original,
        f'if (!_PyIR_HasProperty((uint16_t)handler, _PyIR_H_{prop}))')
manifest = {'interpreter': str(Path(sys.executable).resolve()),
            'interpreter_sha256': hashlib.sha256(Path(sys.executable).read_bytes()).hexdigest(),
            'original_header_sha256': hashlib.sha256(header.encode()).hexdigest(),
            'fixture_sha256': hashlib.sha256(fixture.encode()).hexdigest(),
            'config_sha256': hashlib.sha256((Path(sys.executable).resolve().parent/'pyconfig.h').read_bytes()).hexdigest(),
            'metadata_sha256': hashlib.sha256((repo/'Include/internal/pycore_ir_metadata.h').read_bytes()).hexdigest(),
            'CFLAGS':sysconfig.get_config_var('CFLAGS'), 'variants': {}}
for name, variant in variants.items():
    directory = out / name
    directory.mkdir(exist_ok=True)
    (directory / 'pycore_ir.h').write_text(variant)
    (directory / 'fixture.c').write_text(source)
    patch = ''.join(difflib.unified_diff(header.splitlines(True), variant.splitlines(True),
                        'a/Include/internal/pycore_ir.h', 'b/Include/internal/pycore_ir.h'))
    (directory / 'header.patch').write_text(patch)
    binary = directory / ('_h3_scalar_control' + sysconfig.get_config_var('EXT_SUFFIX'))
    command = (shlex.split(sysconfig.get_config_var('LDSHARED')) +
        shlex.split(sysconfig.get_config_var('CFLAGS')) + ['-fPIC',
        '-I' + str(directory), '-I' + str(Path(sys.executable).resolve().parent),
        '-I' + str(repo / 'Include'), '-I' + str(repo / 'Include/internal'),
        '-MMD', '-MF', str(directory / 'headers.d'), str(directory / 'fixture.c'), '-o', str(binary)])
    subprocess.run(command, check=True)
    dependencies = (directory / 'headers.d').read_text()
    assert str(directory / 'pycore_ir.h') in dependencies
    assert str(repo / 'Include/internal/pycore_ir_metadata.h') in dependencies
    assert str(Path(sys.executable).resolve().parent / 'pyconfig.h') in dependencies
    probe = '''
import importlib.util, json, sys
path = sys.argv[1]
from opcode import opmap
nop = opmap['NOP']
spec = importlib.util.spec_from_file_location('_h3_scalar_control', path)
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
rows = []
for rewrite in (False, True):
    for handler in (-1, 65536, 65536 + nop, nop):
        accepted, before, after = m.ir_scalar_boundary(rewrite, handler)
        rows.append([rewrite, handler, accepted, before, after])
print(json.dumps({'module':m.__file__, 'rows': rows}))
'''
    result = subprocess.run([sys.executable, '-c', probe, str(binary)],
                            text=True, capture_output=True, check=True)
    (directory / 'output.json').write_text(result.stdout)
    rows = json.loads(result.stdout)['rows']
    rejected = all(accepted == 0 and before == after
                   for rewrite, handler, accepted, before, after in rows if handler != opmap['NOP'])
    positives = all(accepted == 1 and before == after
                    for rewrite, handler, accepted, before, after in rows if handler == opmap['NOP'])
    assert positives
    assert rejected == (name == 'positive'), (name, rows)
    if name != 'positive':
        target = name == 'rewrite'
        witnesses = [row for row in rows if row[0] == target and row[1] == 65536 + opmap['NOP']]
        assert len(witnesses) == 1 and witnesses[0][2] == 1
        assert any(row[0] == target and row[1] == 65536 and row[2] == 1 and row[3] != row[4] for row in rows)
    manifest['variants'][name] = {'command': command, 'rejected_bad_scalars': rejected,
        'header_sha256': hashlib.sha256(variant.encode()).hexdigest(),
        'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
        'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
        'patch_sha256': hashlib.sha256(patch.encode()).hexdigest()}
assert header_path.read_text() == header
(out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
print('PASS: positive rejects original scalars; both cast-before-check controls accept 65536+NOP')
