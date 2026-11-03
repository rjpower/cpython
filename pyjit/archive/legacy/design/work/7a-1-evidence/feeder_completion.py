"""Exercise real queue shutdown against the current or reviewed old teardown.

Run with build/debug/python -B this_file {old,current,blocked}.  The old
control must exit 1; current and blocked must exit 0.  No collector/runtime
source switch is used.  blocked keeps the feeder alive after its sentinel
until teardown's single wait budget expires, then explicitly releases it.
"""

import ast
import gc
import multiprocessing
import multiprocessing.queues as queues
from pathlib import Path
import subprocess
import sys
import threading
import time
import weakref
from test import support


mode = sys.argv[1]
assert mode in {"old", "current", "blocked"}
observer = Path(__file__).with_name("trace_mp.py").read_text()
exec(compile(observer.split("args = sys.argv[1:]")[0],
             "<failure-only feeder observer>", "exec"), globals())
if mode == "old":
    # This exact committed body predates the bounded join.  Compile only the
    # method, preserving the globals used by the real multiprocessing tests.
    source = subprocess.check_output(
        ["git", "show", "970e14edbde:Lib/test/_test_multiprocessing.py"], text=True)
    base = next(node for node in ast.parse(source).body
                if isinstance(node, ast.ClassDef) and node.name == "BaseMixin")
    method = next(node for node in base.body
                  if isinstance(node, ast.FunctionDef) and node.name == "tearDownClass")
    method.decorator_list = []
    method.name = "old_teardown_control"
    exec(compile(ast.Module(body=[method], type_ignores=[]),
                 "<970e14edbde BaseMixin.tearDownClass>", "exec"), mp.__dict__)
    original_teardown = mp.old_teardown_control

release = threading.Event()
close_entered = threading.Event()
closed = []
original_close = queues.Queue._finalize_close
old_timeout = support.SHORT_TIMEOUT
old_interval = sys.getswitchinterval()


def record_close(buffer, notempty):
    closed.append("sentinel queued")
    original_close(buffer, notempty)


def make_queue():
    queue = multiprocessing.get_context("forkserver").Queue()
    if mode == "blocked":
        reader_close = queue._reader.close

        def blocked_reader_close():
            close_entered.set()
            release.wait(10)
            reader_close()

        queue._reader.close = blocked_reader_close
    queue.put(1)
    assert queue.get() == 1
    return weakref.ref(queue._thread)


class Probe:
    pass


queues.Queue._finalize_close = staticmethod(record_close)
mp.BaseMixin.setUpClass.__func__(Probe)
sys.setswitchinterval(1.0)
if mode == "blocked":
    support.SHORT_TIMEOUT = 0.1
was_enabled = gc.isenabled()
gc.disable()
thread = None
try:
    thread_ref = make_queue()
    before = time.monotonic()
    mp.BaseMixin.tearDownClass.__func__(Probe)
    elapsed = time.monotonic() - before
    thread = thread_ref()
    alive = thread is not None and thread.is_alive()
    print("SCHEDULE_RESULT", mode, "close_calls", closed,
          "warning", support.environment_altered, "alive", alive,
          "close_entered", close_entered.is_set(), "elapsed", elapsed, flush=True)
    assert closed == ["sentinel queued"]
    if mode == "blocked":
        assert close_entered.is_set() and alive
        assert support.environment_altered, "live feeder escaped leak warning"
        assert 0.08 <= elapsed < 5.0, "teardown did not honor bounded wait"
    else:
        assert not support.environment_altered, "teardown warned before feeder completion"
        assert not alive
finally:
    release.set()
    if thread is not None:
        thread.join(5)
        print("AFTER_JOIN", thread.is_alive(), flush=True)
        assert not thread.is_alive()
    queues.Queue._finalize_close = staticmethod(original_close)
    support.SHORT_TIMEOUT = old_timeout
    sys.setswitchinterval(old_interval)
    if was_enabled:
        gc.enable()
