"""Compile copied current/old safe lookup helpers, never live variants."""

import difflib
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import sys
import sysconfig


def sha(data):
    return hashlib.sha256(data).hexdigest()


def extract(text, start, end):
    begin = text.index(start)
    return text[begin:text.index(end, begin)]


def main():
    repo = next(p for p in Path(__file__).resolve().parents
                if (p / "Objects/codeobject.c").is_file())
    build = Path(sys.executable).resolve().parent
    out = repo / "build/H3-1-stage2b2a" / ("line-controls-" + build.name)
    out.mkdir(parents=True, exist_ok=True)
    baseline = "617bea5be1b1b6b0d368e9ee8c4ca51eb18f7d5d"
    code = (repo / "Objects/codeobject.c").read_text()
    old = subprocess.check_output(["git", "show", baseline + ":Objects/codeobject.c"],
                                  cwd=repo, text=True)
    marker = "int\n_PyCode_SafeAddr2Line("
    end = "\nvoid\n_PyLineTable_InitAddressRange"
    bodies = {"positive": extract(code, marker, end),
              "old-cache": extract(old, marker, end)}
    instrument = (repo / "Python/instrumentation.c").read_text()
    dependencies = (
        extract(instrument, "static int\ncompute_line(", "\nstatic inline uint8_t *\nget_original_opcode_ptr"),
        extract(instrument, "static inline int\nget_line_delta(", "\nstatic inline void\nset_line_delta"),
        extract(instrument, "int\n_Py_Instrumentation_GetLine(", "\nPy_NO_INLINE int\n_Py_call_instrumentation_line"))
    wrapper = '''
static PyObject *lookup(PyObject *self, PyObject *args) {
    PyCodeObject *code;
    int offset;
    if (!PyArg_ParseTuple(args, "O!i", &PyCode_Type, &code, &offset)) return NULL;
    return PyLong_FromLong(copied_safe_line(code, offset));
}
static PyMethodDef methods[] = {
    {"lookup", lookup, METH_VARARGS}, {NULL, NULL, 0, NULL}
};
static struct PyModuleDef definition = {
    PyModuleDef_HEAD_INIT, "_h3_safe_line", NULL, -1, methods
};
PyMODINIT_FUNC PyInit__h3_safe_line(void) { return PyModule_Create(&definition); }
'''
    inputs = [repo / "Objects/codeobject.c", repo / "Python/instrumentation.c",
              repo / "Include/internal/pycore_code.h", repo / "Include/internal/pycore_instruments.h",
              repo / "Lib/test/test_code.py", build / "pyconfig.h", Path(sys.executable).resolve()]
    hashes = {str(path): sha(path.read_bytes()) for path in inputs}
    manifest = {"baseline": baseline, "inputs": hashes,
                "old_codeobject_sha256": sha(old.encode()),
                "decoder_bodies_sha256": [sha(body.encode()) for body in dependencies],
                "compiler": subprocess.check_output(
                    shlex.split(sysconfig.get_config_var("CC")) + ["--version"], text=True),
                "variants": {}}
    for name, body in bodies.items():
        directory = out / name
        directory.mkdir(exist_ok=True)
        copied = body.replace("int\n_PyCode_SafeAddr2Line(", "static int\ncopied_safe_line(")
        prefix = '#define Py_BUILD_CORE_MODULE\n#include "Python.h"\n#include "pycore_code.h"\n#include "pycore_instruments.h"\n'
        if name == "old-cache":
            # These exact old decoder bodies avoid exporting private helpers.
            prefix += '#define NO_LINE (-2)\n' + dependencies[0] + dependencies[1]
            prefix += dependencies[2].replace("int\n_Py_Instrumentation_GetLine(",
                                              "static int\ncopied_monitor_line(")
            copied = copied.replace("_Py_Instrumentation_GetLine(", "copied_monitor_line(")
        source = directory / "fixture.c"
        source.write_text(prefix + copied + wrapper)
        patch = "".join(difflib.unified_diff(
            bodies["positive"].splitlines(True), body.splitlines(True),
            "a/SafeAddr2Line", "b/SafeAddr2Line"))
        (directory / "body.patch").write_text(patch)
        binary = directory / ("_h3_safe_line" + sysconfig.get_config_var("EXT_SUFFIX"))
        command = (shlex.split(sysconfig.get_config_var("LDSHARED"))
                   + shlex.split(sysconfig.get_config_var("CFLAGS"))
                   + ["-fPIC", "-I" + str(build), "-I" + str(repo / "Include"),
                      "-I" + str(repo / "Include/internal"), "-MMD", "-MF",
                      str(directory / "headers.d"), str(source), "-o", str(binary)])
        subprocess.run(command, check=True)
        includes = (directory / "headers.d").read_text()
        for path in (inputs[2], inputs[3], build / "pyconfig.h"):
            assert str(path) in includes, path
        records = {}
        for target in ("loop", "send"):
            probe = Path(__file__).with_name("line_probe.py")
            result = subprocess.run([sys.executable, str(probe), str(binary), target],
                                    text=True, capture_output=True)
            (directory / (target + ".log")).write_text(result.stdout + result.stderr)
            assert result.returncode == (0 if name == "positive" else 1), result
            if name != "positive":
                assert "AssertionError: ('immutable synthetic line'" in result.stderr, result
            records[target] = {"returncode": result.returncode,
                               "probe_sha256": sha(probe.read_bytes())}
        manifest["variants"][name] = {"body_sha256": sha(body.encode()),
            "source_sha256": sha(source.read_bytes()), "binary_sha256": sha(binary.read_bytes()),
            "patch_sha256": sha(patch.encode()), "command": command, "records": records}
    assert hashes == {str(path): sha(path.read_bytes()) for path in inputs}
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print("PASS: copied current helper matches real words; old cache fails both relaxed offsets")


if __name__ == "__main__":
    main()
