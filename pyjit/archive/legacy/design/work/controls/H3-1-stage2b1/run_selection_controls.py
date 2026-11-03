"""Compile copied HandlerFor bodies; leave live source and binaries alone."""

import difflib
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import sys
import sysconfig


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    repo = next(p for p in Path(__file__).resolve().parents
                if (p / "Python/ir.c").is_file())
    build = Path(sys.executable).resolve().parent
    out = repo / "build/H3-1-stage2b1" / ("selection-" + build.name)
    out.mkdir(exist_ok=True)
    path = repo / "Python/ir.c"
    original = path.read_text()
    start = original.index("int\n_PyCode_HandlerFor(")
    end = original.index("\nint\n_PyIR_RealHandler", start)
    body = original[start:end]
    old = '''    // The instrumented twin of a NOFB form is NOFB too
    if (_PyIR_HasProperty(handler, _PyIR_H_NOFB)
        && _PyIR_FormatOf(handler)->fb == _PyIR_LOC_NONE
        && _PyIR_FormatOf(opcode)->nofb != 0)
    {
        return _PyIR_FormatOf(opcode)->nofb;
    }
'''
    assert body.count(old) == 1
    variants = {"positive": body, "omit-nofb": body.replace(old, "")}
    wrapper = '''
static PyObject *select_word(PyObject *self, PyObject *args) {
    PyCodeObject *co;
    int index, installed, restored;
    if (!PyArg_ParseTuple(args, "O!iii", &PyCode_Type, &co,
                          &index, &installed, &restored)) return NULL;
    _PyIRWord word = ((const _PyIRWord *)co->co_words)[index];
    if (!_PyIR_SetHandler(&word, copied_handler_for(co, index, installed))) {
        PyErr_SetString(PyExc_AssertionError, "install rejected");
        return NULL;
    }
    unsigned long long first = word;
    if (!_PyIR_SetHandler(&word, copied_handler_for(co, index, restored))) {
        PyErr_SetString(PyExc_AssertionError, "restore rejected");
        return NULL;
    }
    return Py_BuildValue("KK", first, (unsigned long long)word);
}
static PyMethodDef methods[] = {
    {"select", select_word, METH_VARARGS}, {NULL, NULL, 0, NULL}
};
static struct PyModuleDef definition = {
    PyModuleDef_HEAD_INIT, "_h3_selection", NULL, -1, methods
};
PyMODINIT_FUNC PyInit__h3_selection(void) {
    return PyModule_Create(&definition);
}
'''
    live = [path, repo / "Include/internal/pycore_ir.h",
            repo / "Include/internal/pycore_ir_metadata.h",
            repo / "Include/opcode_ids.h",
            build / "pyconfig.h", Path(sys.executable).resolve()]
    before = {str(p): sha(p) for p in live}
    manifest = {"inputs": before,
                "body_sha256": hashlib.sha256(body.encode()).hexdigest(),
                "compiler": subprocess.check_output(
                    shlex.split(sysconfig.get_config_var("CC")) + ["--version"],
                    text=True), "variants": {}}
    for name, variant in variants.items():
        directory = out / name
        directory.mkdir(exist_ok=True)
        source = directory / "fixture.c"
        source.write_text('#define Py_BUILD_CORE_MODULE\n#include "Python.h"\n'
                          '#include "pycore_ir.h"\n' + variant.replace(
                              "int\n_PyCode_HandlerFor(",
                              "static int\ncopied_handler_for(") + wrapper)
        patch = "".join(difflib.unified_diff(
            body.splitlines(True), variant.splitlines(True),
            "a/HandlerFor", "b/HandlerFor"))
        (directory / "body.patch").write_text(patch)
        binary = directory / ("_h3_selection" + sysconfig.get_config_var("EXT_SUFFIX"))
        command = (shlex.split(sysconfig.get_config_var("LDSHARED"))
                   + shlex.split(sysconfig.get_config_var("CFLAGS"))
                   + ["-fPIC", "-I" + str(build), "-I" + str(repo / "Include"),
                      "-I" + str(repo / "Include/internal"), "-MMD", "-MF",
                      str(directory / "headers.d"), str(source), "-o", str(binary)])
        subprocess.run(command, check=True)
        dependencies = (directory / "headers.d").read_text()
        for header in live[1:-1]:
            assert str(header) in dependencies, header
        probe = Path(__file__).with_name("selection_probe.py")
        result = subprocess.run([sys.executable, str(probe), str(binary)],
                                text=True, capture_output=True)
        (directory / "output.log").write_text(result.stdout + result.stderr)
        assert result.returncode == (0 if name == "positive" else 1), result
        if name != "positive":
            assert "AssertionError: ('installed NOFB twin'" in result.stderr, result
        manifest["variants"][name] = {
            "command": command, "returncode": result.returncode,
            "source_sha256": sha(source), "binary_sha256": sha(binary),
            "patch_sha256": sha(directory / "body.patch"),
            "probe_sha256": sha(probe)}
    assert before == {str(p): sha(p) for p in live}
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print("PASS: live twin/restoration matches copied positive; omitted NOFB selection fails")


if __name__ == "__main__":
    main()
