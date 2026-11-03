"""Exercise gate lock ownership without sending signals to real test workers."""

import fcntl
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time


def wait_for(predicate):
    deadline = time.monotonic() + 5
    while not predicate():
        if time.monotonic() >= deadline:
            raise AssertionError("gate test did not reach its checkpoint")
        time.sleep(0.01)


def start_gate(runner, root, name, command):
    env = dict(os.environ, PATH=f"{root}:{os.environ['PATH']}",
               GATE_TEST_DIR=str(root), GATE_TEST_NAME=name)
    return subprocess.Popen(
        ["sh", str(runner), str(root / "lock"), str(root / "python"),
         "sh", "-c", command], env=env, start_new_session=True,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    )


def stop(process):
    if process.poll() is None:
        os.killpg(process.pid, signal.SIGKILL)
    process.communicate(timeout=5)


def waiting_on_flock(process):
    children = Path(f"/proc/{process.pid}/task/{process.pid}/children")
    for pid in children.read_text().split():
        if Path(f"/proc/{pid}/comm").read_text().strip() == "flock":
            return True
    return False


def test_cleanup_owns_lock(runner, root):
    first = start_gate(runner, root, "first", ":")
    second = None
    try:
        wait_for((root / "cleanup-first").exists)
        with (root / "lock").open("w") as lock:
            try:
                fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            except BlockingIOError:
                pass
            else:
                raise AssertionError("build lock released before sweep finished")
        second = start_gate(runner, root, "second",
                            'touch "$GATE_TEST_DIR/second-started"')
        time.sleep(0.1)
        assert not (root / "second-started").exists()
        (root / "release-first").touch()
        assert first.wait(timeout=5) == 0
        assert second.wait(timeout=5) == 0
        assert (root / "second-started").exists()
    finally:
        (root / "release-first").touch()
        stop(first)
        if second is not None:
            stop(second)


def test_waiter_cannot_sweep(runner, root):
    owner = start_gate(runner, root, "owner",
                       'touch "$GATE_TEST_DIR/owner-started"; '
                       'while ! test -e "$GATE_TEST_DIR/release-owner"; '
                       'do sleep 0.01; done')
    waiter = None
    try:
        wait_for((root / "owner-started").exists)
        waiter = start_gate(runner, root, "waiter", ":")
        wait_for(lambda: waiting_on_flock(waiter))
        os.killpg(waiter.pid, signal.SIGTERM)
        assert waiter.wait(timeout=5) == 143
        assert not (root / "cleanup-waiter").exists(), "waiter swept owner's workers"
        assert owner.poll() is None
        (root / "release-owner").touch()
        assert owner.wait(timeout=5) == 0
    finally:
        (root / "release-owner").touch()
        stop(owner)
        if waiter is not None:
            stop(waiter)


def test_failure_status(runner, root):
    process = start_gate(runner, root, "failure", "exit 7")
    try:
        assert process.wait(timeout=5) == 7
        assert (root / "cleanup-failure").exists()
        with (root / "lock").open("w") as lock:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    finally:
        stop(process)


def main():
    runner = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else (
        Path(__file__).with_name("gate_run.sh")
    )
    with tempfile.TemporaryDirectory(prefix="boil-gate-lock-") as directory:
        root = Path(directory)
        # A controlled sweep checkpoint replaces pkill; no real worker is killed.
        pkill = root / "pkill"
        pkill.write_text(
            '#!/bin/sh\n'
            'touch "$GATE_TEST_DIR/cleanup-$GATE_TEST_NAME"\n'
            'if [ "$GATE_TEST_NAME" = first ]; then\n'
            '  while ! test -e "$GATE_TEST_DIR/release-first"; do sleep 0.01; done\n'
            'fi\n'
            'exit 1\n'
        )
        pkill.chmod(0o755)
        for test in (test_cleanup_owns_lock, test_waiter_cannot_sweep, test_failure_status):
            test(runner, root)
            print(f"{test.__name__}: passed", flush=True)


if __name__ == "__main__":
    main()
