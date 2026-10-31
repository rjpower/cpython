"""Semantic metadata controls in copied generators/Lib, never live source."""

import difflib
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    repo = next(p for p in Path(__file__).resolve().parents
                if (p / "Python/ir.c").is_file())
    out = repo / "build/H3-1-stage2b1" / ("metadata-" + Path(sys.executable).parent.name)
    out.mkdir(exist_ok=True)
    generator = Path("Tools/cases_generator/opcode_metadata_generator.py")
    copied = list(Path("Tools/cases_generator").glob("*.py"))
    copied += [Path("Python/bytecodes.c"), Path("Include/opcode.h")]
    copied += [Path("Lib") / p for p in ("dis.py", "opcode.py", "_opcode_metadata.py")]
    before = {str(p): sha(repo / p) for p in copied}
    variants = [("positive", None, ""), ("name", generator, ""),
                ("body", Path("Tools/cases_generator/ir_generator.py"), ""),
                ("operation", Path("Lib/_opcode_metadata.py"), '''
_handler_operations = tuple(10 if i == _ir_opmap['BINARY_OP_ADD'] else value
                            for i, value in enumerate(_handler_operations))
'''), ("kind", Path("Lib/_opcode_metadata.py"), '''
_handler_flags = tuple(value | _HANDLER_SPECIALIZED
                       if i == _ir_opmap['BINARY_OP_ADD'] else value
                       for i, value in enumerate(_handler_flags))
''')]
    probe = '''
import hashlib, io, json, pathlib, sys
root = pathlib.Path(sys.argv[1])
sys.path.insert(0, str(root / 'Tools/cases_generator'))
import opcode_metadata_generator as generator
import analyzer, ir_generator
import dis, opcode, _opcode_metadata
paths = {}
for module in (generator, analyzer, ir_generator, dis, opcode, _opcode_metadata):
    path = pathlib.Path(module.__file__).resolve()
    assert path.is_relative_to(root), (module.__name__, path)
    paths[module.__name__] = [str(path), hashlib.sha256(path.read_bytes()).hexdigest()]
analysis = analyzer.analyze_files([str(root / 'Python/bytecodes.c')])
domain = ir_generator.build_handler_domain(analysis)
from cwriter import CWriter
stream = io.StringIO()
generator.generate_name_table(analysis, domain, CWriter(stream, 0, False))
name = f'[{domain.handlers["IR_JUMP"]}] = "IR_JUMP",'
assert name in stream.getvalue(), ('full-domain name', name)
cases = io.StringIO()
ir_generator.generate_ir_cases(analysis, domain,
                              ir_generator.compute_formats(analysis), cases, False)
body = cases.getvalue().split('TARGET(INSTRUMENTED_LOAD_SUPER_ATTR_NOFB) {', 1)[1].split('TARGET(', 1)[0]
lines = [line.strip() for line in body.splitlines()]
assert 'int opcode = INSTRUMENTED_LOAD_SUPER_ATTR_NOFB;' in lines
assert 'opcode = INSTRUMENTED_LOAD_SUPER_ATTR;' in lines, ('semantic body opcode', lines[:25])
def f(a, b):
    return a + b
assert f(3, 4) == 7
item = next(i for i in dis.get_instructions(f, adaptive=True)
            if i.handler == 'BINARY_OP_ADD')
assert item.arg == 0, ('operation metadata', item)
assert item.opname == 'BINARY_OP', ('specialization kind', item)
print(json.dumps({'paths': paths, 'name': name,
                  'instruction': [item.handler, item.opname, item.arg]}))
'''
    manifest = {"inputs": before, "interpreter": str(Path(sys.executable).resolve()),
                "interpreter_sha256": sha(Path(sys.executable)), "variants": {}}
    for name, changed, suffix in variants:
        directory = out / name
        directory.mkdir(exist_ok=True)
        for relative in copied:
            target = directory / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(repo / relative, target)
        patch = ""
        if changed:
            original = (repo / changed).read_text()
            if name == "name":
                old = '        if domain.kinds[name] & HandlerKind.METADATA:\n'
                assert original.count(old) == 1
                # Suppress the real IR-only name; leave parsing and IDs intact.
                variant = original.replace(old,
                    '        if domain.kinds[name] & HandlerKind.METADATA and name != "IR_JUMP":\n')
            elif name == "body":
                old = '        out.emit(f"opcode = {inst.name};\\n")\n'
                assert original.count(old) == 1
                variant = original.replace(old,
                    '        out.emit(f"opcode = {name};\\n")\n')
            else:
                variant = original + suffix
            (directory / changed).write_text(variant)
            patch = "".join(difflib.unified_diff(
                original.splitlines(True), variant.splitlines(True),
                "a/" + str(changed), "b/" + str(changed)))
        (directory / "metadata.patch").write_text(patch)
        env = dict(os.environ, PYTHONPATH=str(directory / "Lib"),
                   PYTHONPYCACHEPREFIX=str(directory / "pyc"))
        result = subprocess.run([sys.executable, "-c", probe, str(directory)],
                                env=env, text=True, capture_output=True)
        (directory / "output.log").write_text(result.stdout + result.stderr)
        assert result.returncode == (0 if name == "positive" else 1), result
        if name != "positive":
            label = {"name": "full-domain name", "body": "semantic body opcode",
                     "operation": "operation metadata",
                     "kind": "specialization kind"}[name]
            assert "AssertionError: ('" + label + "'" in result.stderr, result
        manifest["variants"][name] = {
            "returncode": result.returncode,
            "files": {str(p): sha(directory / p) for p in copied},
            "patch_sha256": sha(directory / "metadata.patch")}
    assert before == {str(p): sha(repo / p) for p in copied}
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print("PASS: copied names/body/operations/kind controls fail semantic assertions")


if __name__ == "__main__":
    main()
