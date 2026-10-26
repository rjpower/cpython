"""Tests for blocking mode sampling profiler."""

import os
import subprocess
import sys
import textwrap
import time
import unittest

try:
    import _remote_debugging  # noqa: F401
    import profiling.sampling
except ImportError:
    raise unittest.SkipTest(
        "Test only runs when _remote_debugging is available"
    )

from test.support import (
    SHORT_TIMEOUT,
    os_helper,
    requires_remote_subprocess_debugging,
)

from .helpers import test_subprocess

@requires_remote_subprocess_debugging()
@unittest.skipUnless(sys.platform == "win32", "Windows only")
class TestBlockingModeCLI(unittest.TestCase):
    def test_run_blocking_exits_after_target_process_exits(self):
        script = 'print("done")\n'

        tmpdir = os.path.abspath(os_helper.TESTFN + "_profiling_blocking")
        with os_helper.temp_dir(tmpdir) as tmpdir:
            script_path = os.path.join(tmpdir, "tiny_target.py")
            profile_path = os.path.join(tmpdir, "blocking.bin")
            with open(script_path, "w", encoding="utf-8") as file:
                file.write(script)

            cmd = [
                sys.executable, "-m", "profiling.sampling", "run",
                "--binary", "-o", profile_path,
                "--mode=cpu", "--blocking", "-r", "100",
                script_path,
            ]
            result = subprocess.run(
                cmd,
                cwd=tmpdir,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                timeout=SHORT_TIMEOUT,
            )

            self.assertEqual(
                result.returncode, 0,
                f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}",
            )
            self.assertGreater(os.path.getsize(profile_path), 0)

            replay = subprocess.run(
                [sys.executable, "-m", "profiling.sampling", "replay",
                 profile_path],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                timeout=SHORT_TIMEOUT,
            )
            self.assertEqual(
                replay.returncode, 0,
                f"stdout:\n{replay.stdout}\nstderr:\n{replay.stderr}",
            )


@requires_remote_subprocess_debugging()
@unittest.skipUnless(sys.platform == "win32", "Windows only")
class TestBlockingModeSuspension(unittest.TestCase):
    def test_all_threads_stop_before_pause_returns(self):
        import mmap

        tag = f"cpython_blocking_{os.getpid()}_{id(self)}"
        script = textwrap.dedent(f'''
            import mmap
            import struct
            import threading

            memory = mmap.mmap(-1, 16, tagname={tag!r})

            def worker(offset):
                counter = 0
                while True:
                    counter += 1
                    struct.pack_into("q", memory, offset, counter)

            for offset in (0, 8):
                threading.Thread(target=worker, args=(offset,), daemon=True).start()
            _test_sock.sendall(b"working")
            _test_sock.recv(1)
        ''')
        with mmap.mmap(-1, 16, tagname=tag) as memory:
            with test_subprocess(script, wait_for_working=True) as subproc:
                unwinder = _remote_debugging.RemoteUnwinder(
                    subproc.process.pid, all_threads=True)
                deadline = time.monotonic() + SHORT_TIMEOUT
                while not all(memory[offset:offset + 8] != bytes(8)
                              for offset in (0, 8)):
                    self.assertLess(time.monotonic(), deadline,
                                    "Worker threads did not start")
                    time.sleep(0.001)
                for _ in range(100):
                    self.assertTrue(unwinder.pause_threads())
                    try:
                        before = memory[:]
                        self.assertFalse(unwinder.pause_threads())
                        unwinder.get_stack_trace()
                        time.sleep(0.001)
                        self.assertEqual(memory[:], before,
                                         "Target memory changed while paused")
                    finally:
                        unwinder.resume_threads()
                    self.assertFalse(unwinder.resume_threads())
                before = memory[:]
                deadline = time.monotonic() + SHORT_TIMEOUT
                while any(memory[offset:offset + 8] == before[offset:offset + 8]
                          for offset in (0, 8)):
                    self.assertLess(time.monotonic(), deadline,
                                    "Worker threads did not resume")
                    time.sleep(0.001)
