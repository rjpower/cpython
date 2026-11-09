"""Compile isolated helper copies; never mutate live headers/source."""
import difflib
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import sysconfig
import tempfile


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def function(text, signature):
    start = text.index(signature)
    return text[start:text.index("\n}\n", start) + 3]


def main(label):
    repo = next(p for p in Path(__file__).resolve().parents if (p / "Python/bytecodes.c").exists())
    directory = Path(__file__).parent
    build = Path(sys.executable).resolve().parent
    out = directory / (label + "-storage-" + build.name)
    out.mkdir(exist_ok=True)
    header_path = repo / "Include/internal/pycore_instruments.h"
    capi_path = repo / "Modules/_testinternalcapi.c"
    instrumentation = repo / "Python/instrumentation.c"
    header = header_path.read_text()
    roundtrip = function(capi_path.read_text(), "static PyObject *\nmonitoring_storage_roundtrip(")
    deinstrument = function(instrumentation.read_text(), "int\n_PyCode_Deinstrument(")
    live = [header_path, capi_path, instrumentation, build / "python", build / "pyconfig.h"]
    before = {str(path): sha(path) for path in live}
    manifest = {"before": before, "driver_sha256": sha(Path(__file__)),
                "probe_sha256": sha(directory / "storage_probe.py"), "fixture_body_sha256": hashlib.sha256(roundtrip.encode()).hexdigest(),
                "deinstrument_body_sha256": hashlib.sha256(deinstrument.encode()).hexdigest(), "variants": {}}
    wrapper = '''
static PyObject *deinstrument_wrapper(PyObject *self, PyObject *args) {
    int opcode;
    if (!PyArg_ParseTuple(args, "i", &opcode)) { return NULL; }
    return PyLong_FromLong(copied_deinstrument(opcode));
}
static PyMethodDef methods[] = {
    {"roundtrip", monitoring_storage_roundtrip, METH_VARARGS, NULL},
    {"deinstrument", deinstrument_wrapper, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}
};
static struct PyModuleDef module = {PyModuleDef_HEAD_INIT, "_h3_storage", NULL, -1, methods};
PyMODINIT_FUNC PyInit__h3_storage(void) { return PyModule_Create(&module); }
'''
    for name in ("positive", "token_truncate", "line_truncate", "raw_alias"):
        variant = out / name
        variant.mkdir(exist_ok=True)
        copied_header, body = header, deinstrument
        if name == "token_truncate":
            copied_header = header.replace("line_data->original_opcodes[index] = opcode;",
                                           "line_data->original_opcodes[index] = (uint8_t)opcode;")
        elif name == "line_truncate":
            copied_header = header.replace("line_data->lines[index] = line;",
                                           "line_data->lines[index] = (uint16_t)line;")
        elif name == "raw_alias":
            body = body.replace("return _PyIR_Deinstrument[_PyIR_BaseOpcode(opcode)];",
                                "return _PyIR_Deinstrument[opcode];")
        if name != "positive":
            assert copied_header != header or body != deinstrument
        copy = variant / "pycore_instruments.h"
        copy.write_text(copied_header)
        (variant / "header.patch").write_text("".join(difflib.unified_diff(
            header.splitlines(True), copied_header.splitlines(True), "a/header", "b/header")))
        (variant / "body.patch").write_text("".join(difflib.unified_diff(
            deinstrument.splitlines(True), body.splitlines(True), "a/deinstrument", "b/deinstrument")))
        source = variant / "fixture.c"
        source.write_text('#define Py_BUILD_CORE_MODULE\n#undef NDEBUG\n#include "Python.h"\n#include "pycore_instruments.h"\n'
                          '#include "pycore_ir.h"\n' + roundtrip + '\n' + body.replace(
                              "int\n_PyCode_Deinstrument(", "static int\ncopied_deinstrument(") + wrapper)
        extension = variant / ("_h3_storage" + sysconfig.get_config_var("EXT_SUFFIX"))
        command = (shlex.split(sysconfig.get_config_var("LDSHARED"))
                   + shlex.split(sysconfig.get_config_var("CFLAGS"))
                   + ["-std=c11", "-fPIC", "-I" + str(variant), "-I" + str(build), "-I" + str(repo / "Include"),
                      "-I" + str(repo / "Include/internal"), "-MMD", "-MF", str(variant / "headers.d"),
                      str(source), "-o", str(extension)])
        subprocess.run(command, check=True, timeout=120)
        dependencies = shlex.split((variant / "headers.d").read_text().replace("\\\n", " ").split(":", 1)[1])
        assert str(copy) in dependencies
        assert str(header_path) not in dependencies
        assert str(build / "pyconfig.h") in dependencies
        env = {k: v for k, v in os.environ.items() if not k.startswith("PYTHON_GC") and k != "PYTHONOPTIMIZE"}
        with tempfile.TemporaryDirectory(prefix="pycache-", dir=variant) as cache:
            result = subprocess.run([sys.executable, "-X", "pycache_prefix=" + cache,
                                     str(directory / "storage_probe.py"), str(extension)],
                                    text=True, capture_output=True, env=env, timeout=120)
        (variant / "output.log").write_text(result.stdout + result.stderr)
        assert result.returncode == (0 if name == "positive" else 1), result.stdout + result.stderr
        if name != "positive":
            expected = "canonical alias" if name == "raw_alias" else "adjacent storage values"
            assert "AssertionError: ('" + expected in result.stderr, result.stderr
        manifest["variants"][name] = {"command": command, "returncode": result.returncode,
            "source_sha256": sha(source), "header_sha256": sha(copy), "extension_sha256": sha(extension),
            "header_patch_sha256": sha(variant / "header.patch"), "body_patch_sha256": sha(variant / "body.patch"),
            "consumed_headers": {name: sha(Path(name)) for name in dependencies},
            "output_sha256": sha(variant / "output.log")}
        print(build.name, name, result.returncode, flush=True)
    assert before == {str(path): sha(path) for path in live}
    manifest["after"] = before
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
    main(sys.argv[1])
