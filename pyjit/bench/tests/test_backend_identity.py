"""Host-only backend probes and retained ELF mapping; no target process runs."""

import contextlib
import importlib.util
import io
import json
from pathlib import Path
import struct
import subprocess
import sys
import sysconfig
import tempfile
import types
import unittest
from unittest import mock


BENCH = Path(__file__).resolve().parents[1]


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


runner = load("backend_bench_runner", BENCH / "run.py")
capture = load("backend_native_capture", BENCH.parent / "issues/call-model55/capture-native.py")
measure = load("backend_checkpoint_measure", BENCH.parent / "issues/checkpoint-closure/measure.py")


def config(backend=None, convention=None, ir=None, gotos=None):
    return dict(PY_INTERPRETER_BACKEND=backend, PY_TAIL_CALL_CC=convention,
                Py_IR_INTERP=ir, HAVE_COMPUTED_GOTOS=gotos)


def elf_image(*, target=0x1020, relocation=8):
    """A PIE fixture with two label pointers supplied by RELATIVE addends."""
    image = bytearray(0x480)
    ident = b"\x7fELF\x02\x01\x01" + bytes(9)
    struct.pack_into("<16sHHIQQQIHHHHHH", image, 0,
                     ident, 3, 62, 1, 0, 0, 0x300, 0, 64, 0, 0, 64, 6, 0)
    strings = b"\0run_handlers\0instruction_targets.7\0"
    image[0x1a0:0x1a0 + len(strings)] = strings
    struct.pack_into("<IBBHQQ", image, 0x218,
                     strings.index(b"run_handlers"), 2, 0, 1, 0x1000, 0x80)
    struct.pack_into("<IBBHQQ", image, 0x230,
                     strings.index(b"instruction_targets"), 1, 0, 2, 0x2000, 16)
    struct.pack_into("<QQq", image, 0x250, 0x2000, 8, 0x1010)
    struct.pack_into("<QQq", image, 0x268, 0x2008, relocation, target)
    sections = [
        (0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        (0, 1, 6, 0x1000, 0x100, 0x80, 0, 0, 16, 0),
        (0, 1, 3, 0x2000, 0x180, 16, 0, 0, 8, 0),
        (0, 3, 0, 0, 0x1a0, len(strings), 0, 0, 1, 0),
        (0, 2, 0, 0, 0x200, 72, 3, 0, 8, 24),
        (0, 4, 0, 0, 0x250, 48, 0, 0, 8, 24),
    ]
    for index, section in enumerate(sections):
        struct.pack_into("<IIQQQQIIQQ", image, 0x300 + index * 64, *section)
    return image


class BackendIdentityTests(unittest.TestCase):
    def probe(self, configuration):
        process = subprocess.CompletedProcess([], 0, json.dumps(
            ["test version", ["CPython", "test", "123456789ab"], 0, configuration]))
        with mock.patch.object(runner, "run_checked", return_value=process) as call:
            result = runner.build_info(Path("/candidate/python"), "candidate")
        self.assertEqual(call.call_count, 1)
        return result

    def test_candidate_and_immutable_controls_keep_raw_identity(self):
        cases = (config("computed_goto", ir=1, gotos=1),
                 config(convention="no_callee_saved_registers", ir=1),
                 config(convention="preserve_none", ir=1), config())
        for configuration in cases:
            with self.subTest(configuration=configuration):
                result = self.probe(configuration)
                self.assertEqual(result["interpreter_backend"],
                                 configuration["PY_INTERPRETER_BACKEND"])
                self.assertEqual(result["tail_call_cc"], configuration["PY_TAIL_CALL_CC"])

    def test_unqualified_or_contradictory_backend_is_rejected(self):
        for configuration in (config(convention="none"), config("unknown"),
                              config("computed_goto", "none", 1, 1),
                              config("computed_goto", ir=1, gotos=0),
                              config("computed_goto", ir=0, gotos=1),
                              config(ir=1), config(convention="unknown")):
            with self.subTest(configuration=configuration):
                with self.assertRaises(runner.MeasurementError):
                    self.probe(configuration)

    def test_checkpoint_probe_retains_backend_configuration(self):
        expected = config("computed_goto", ir=1, gotos=1) | {"CFLAGS_CEVAL": ""}
        output = io.StringIO()
        with mock.patch.object(sysconfig, "get_config_var", side_effect=expected.get), \
                mock.patch.dict(sys.modules, {"_testinternalcapi": types.ModuleType("_testinternalcapi")}), \
                contextlib.redirect_stdout(output):
            exec(measure._probe_source(), {})
        actual = json.loads(output.getvalue())["config"]
        self.assertEqual({name: actual[name] for name in expected}, expected)


class EngineTargetTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="backend-identity-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        (self.root / "Python").mkdir()
        self.header = self.root / "Python/ir_targets.h"
        self.header.write_text("static void *const instruction_targets[2] = {\n"
                               "[0] = &&TARGET_UNKNOWN_OPCODE,\n"
                               "[1] = &&TARGET_LOAD_CONST,\n};\n")
        self.executable = self.root / "python"

    def test_compiler_suffix_and_relative_relocations_resolve_every_target(self):
        self.executable.write_bytes(elf_image())
        result = capture.engine_targets(self.executable, self.root)
        self.assertEqual(result["table"]["symbol"], "instruction_targets.7")
        self.assertEqual(result["table"]["relative_relocations"], 2)
        self.assertEqual([(r["handler"], r["label"], r["address"])
                          for r in result["targets"]],
                         [(0, "UNKNOWN_OPCODE", 0x1010), (1, "LOAD_CONST", 0x1020)])

    def test_outside_target_unsupported_relocation_and_truncation_fail(self):
        for image in (elf_image(target=0x1080), elf_image(relocation=1),
                      elf_image()[:0x400]):
            with self.subTest(size=len(image)):
                self.executable.write_bytes(image)
                with self.assertRaises(ValueError):
                    capture.engine_targets(self.executable, self.root)

    def test_missing_generated_entry_and_section_bounds_fail(self):
        self.executable.write_bytes(elf_image())
        original = self.header.read_text()
        self.header.write_text(original.replace("[1] =", "[2] ="))
        with self.assertRaisesRegex(ValueError, "complete ordered"):
            capture.engine_targets(self.executable, self.root)
        self.header.write_text(original)
        image = elf_image()
        struct.pack_into("<Q", image, 0x300 + 64 + 16, 0x1040)
        self.executable.write_bytes(image)
        with self.assertRaisesRegex(ValueError, "section bounds"):
            capture.engine_targets(self.executable, self.root)


if __name__ == "__main__":
    unittest.main()
