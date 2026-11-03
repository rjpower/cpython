"""Test in-process C stack unwinders against Python frames.

The tests build a recursive Python call stack, ask each _testinternalcapi
unwinder for return addresses, and classify those addresses as Python or
other frames. The backends include CPython's manual stack-chain unwinder and
GNU backtrace(), so this module is about in-process C stack unwinding rather
than a single unwind mechanism. GDB integration tests live in test_gdb.
"""

import json
import platform
import subprocess
import sys
import sysconfig
import unittest

from test import support
from test.support import import_helper


_testinternalcapi = import_helper.import_module("_testinternalcapi")


if not support.has_subprocess_support:
    raise unittest.SkipTest("test requires subprocess support")


STACK_DEPTH = 10


def _manual_unwind_expected(machine):
    _Py_WITH_FRAME_POINTERS = getattr(
        _testinternalcapi,
        "_Py_WITH_FRAME_POINTERS",
        -1,
    )
    if _Py_WITH_FRAME_POINTERS > 0:
        return True
    if _Py_WITH_FRAME_POINTERS == 0:
        return False

    cflags = " ".join(
        value for value in (
            sysconfig.get_config_var("PY_CORE_CFLAGS"),
            sysconfig.get_config_var("CFLAGS"),
        )
        if value
    )

    if "no-omit-frame-pointer" in cflags:
        # For example, configure adds -fno-omit-frame-pointer by default on
        # supported GCC-compatible builds.
        return True
    if "omit-frame-pointer" in cflags:
        return False

    if sys.platform == "darwin":
        # macOS x86_64/ARM64 always have frame pointer by default.
        return True

    if sys.platform == "linux":
        if machine in {"aarch64", "arm64"}:
            # 32-bit Linux is not supported
            if sys.maxsize < 2**32:
                return None
            return True
        if machine == "ppc64le":
            # The power ABI specification requires that compilers maintain a
            # back chain by default, so unwinding already works without a
            # dedicated frame pointer.
            # https://openpowerfoundation.org/specifications/64bitelfabi/
            return True
        if machine == "x86_64":
            final_opt = ""
            for opt in cflags.split():
                if opt.startswith('-O'):
                    final_opt = opt
            if final_opt in ("-O0", "-Og", "-O1"):
                # Unwinding works if the optimization level is low
                return True

            Py_ENABLE_SHARED = int(sysconfig.get_config_var('Py_ENABLE_SHARED') or '0')
            if Py_ENABLE_SHARED:
                # Unwinding does crash using gcc -O2 or gcc -O3
                # when Python is built with --enable-shared
                return "crash"
            return False

    if sys.platform == "win32":
        # MSVC ignores /Oy and /Oy- on x64/ARM64.
        if machine == "arm64":
            # Windows ARM64 guidelines recommend frame pointers (x29) for stack walking.
            return True
        elif machine == "x86_64":
            # Windows x64 uses unwind metadata; frame pointers are not required.
            return None
    return None


def _is_arm32_build():
    if sys.maxsize >= 2**32:
        return False

    abi = " ".join(
        value for value in (
            sysconfig.get_config_var("MULTIARCH"),
            sysconfig.get_config_var("HOST_GNU_TYPE"),
            sysconfig.get_config_var("SOABI"),
        )
        if value
    ).lower()
    return "arm" in abi


def _build_stack_and_unwind(unwinder):
    import operator

    def build_stack(n, unwinder):
        if n == 0:
            return unwinder()
        return operator.call(build_stack, n - 1, unwinder)

    stack = build_stack(STACK_DEPTH, unwinder)
    return stack


def _classify_stack(stack):
    labels = _testinternalcapi.classify_stack_addresses(stack)

    annotated = []
    python_frames = 0
    other_frames = 0
    for idx, (frame, tag) in enumerate(zip(stack, labels)):
        addr = int(frame)
        if tag == "python":
            python_frames += 1
        else:
            other_frames += 1
        annotated.append((idx, addr, tag))
    return annotated, python_frames, other_frames


def _summarize_unwind(stack, unwinder_name):
    annotated, python_frames, other_frames = _classify_stack(stack)
    for idx, addr, tag in annotated:
        print(f"#{idx:02d} {addr:#x} -> {tag}")
    return {
        "length": len(stack),
        "python_frames": python_frames,
        "other_frames": other_frames,
        "unwinder": unwinder_name,
    }


def _annotate_unwind(unwinder_name="manual_frame_pointer_unwind"):
    unwinder = getattr(_testinternalcapi, unwinder_name)
    stack = _build_stack_and_unwind(unwinder)
    return json.dumps(_summarize_unwind(stack, unwinder_name))


def _run_unwind_helper(helper_name, unwinder_name):
    code = (
        f"from test.test_c_stack_unwind import {helper_name}; "
        f"print({helper_name}({unwinder_name!r}));"
    )
    proc = subprocess.run(
        [sys.executable, "-c", code],
        capture_output=True,
        text=True,
    )
    # Surface the output for debugging/visibility when running this test
    if proc.stdout:
        print(proc.stdout, end="")
    if proc.returncode:
        raise RuntimeError(
            f"unwind helper failed (rc={proc.returncode}): {proc.stderr or proc.stdout}"
        )
    stdout_lines = proc.stdout.strip().splitlines()
    if not stdout_lines:
        raise RuntimeError("unwind helper produced no output")
    try:
        return json.loads(stdout_lines[-1])
    except ValueError as exc:
        raise RuntimeError(
            f"unexpected output from unwind helper: {proc.stdout!r}"
        ) from exc


def _unwind_result(unwinder_name):
    return _run_unwind_helper("_annotate_unwind", unwinder_name)


@support.requires_gil_enabled("test requires the GIL enabled")
@unittest.skipIf(support.is_wasi, "test not supported on WASI")
class ManualStackUnwindTests(unittest.TestCase):

    def setUp(self):
        super().setUp()

        machine = platform.machine().lower()
        expected = _manual_unwind_expected(machine)
        if expected is None:
            self.skipTest(
                f"unsupported architecture for manual stack unwind check: {machine}"
            )
        if expected == "crash":
            self.skipTest(f"test does crash on {machine}")

        try:
            _testinternalcapi.manual_frame_pointer_unwind()
        except RuntimeError as exc:
            if "not supported" in str(exc):
                self.skipTest(
                    "manual stack unwinding not supported on this platform"
                )
            raise
        self.machine = machine
        self.manual_unwind_expected = expected

    def test_manual_unwind_finds_expected_frames(self):
        result = _unwind_result("manual_frame_pointer_unwind")
        python_frames = result.get("python_frames", 0)
        if self.manual_unwind_expected:
            self.assertGreaterEqual(
                python_frames,
                STACK_DEPTH,
                f"expected to find Python frames on {self.machine}",
            )
        else:
            self.assertLessEqual(
                python_frames,
                1,
                f"unexpected Python frames counted on {self.machine}",
            )


@support.requires_gil_enabled("test requires the GIL enabled")
@unittest.skipIf(support.is_wasi, "test not supported on WASI")
@unittest.skipUnless(sys.platform == "linux", "GNU backtrace unwinding test requires Linux")
@unittest.skipIf(
    _is_arm32_build(),
    "GNU backtrace unwinding skipped on Arm 32-bit",
)
class GnuBacktraceUnwindTests(unittest.TestCase):

    def setUp(self):
        super().setUp()
        try:
            _testinternalcapi.gnu_backtrace_unwind()
        except RuntimeError as exc:
            if "not supported" in str(exc):
                self.skipTest("gnu backtrace unwinding not supported on this platform")
            raise

    def test_gnu_backtrace_finds_python_frames(self):
        result = _unwind_result("gnu_backtrace_unwind")
        self.assertGreaterEqual(
            result.get("python_frames", 0),
            STACK_DEPTH,
            "expected to find Python frames in GNU backtrace",
        )


if __name__ == "__main__":
    unittest.main()
