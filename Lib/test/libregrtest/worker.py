import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass
from _colorize import can_colorize  # type: ignore[import-not-found]
from typing import Any, NoReturn

from test.support import os_helper, Py_DEBUG

from .setup import setup_process, setup_test_dir
from .runtests import WorkerRunTests, JsonFile, JsonFileType
from .single import run_single_test
from .result import State
from .utils import (
    StrPath, StrJSON, TestFilter,
    get_temp_dir, get_work_dir, exit_timeout)


USE_PROCESS_GROUP = (hasattr(os, "setsid") and hasattr(os, "killpg"))
NEED_TTY = {
    'test_ioctl',
}


# Stats builds: record which launches of a build product will count.
#
# The hook writes one JSON line per launch to <dir>/<tag>.spawn. The
# stats gate (Tools/build/check_r27.py) matches the launches predicted to
# count with the .run files and dumps of their children (R2-2c), and lists
# the others.

_SPAWN_EVENTS = {
    'subprocess.Popen', 'os.posix_spawn', 'os.exec', 'os.spawn', 'os.system',
}
# Short options that take a separate argument (python -X NAME, -W ARG).
_ARG_OPTIONS = {'-X', '-W', '-Q'}


def _stats_dir() -> str | None:
    value = sys._xoptions.get('pystats_dir') or os.environ.get('PYTHON_PYSTATS_DIR')
    if isinstance(value, str) and os.path.isdir(value):
        return value
    return None


def _build_products() -> list[str]:
    """Paths of the build's executables: python and Programs/_testembed."""
    exe = sys.executable
    paths = [exe, os.path.realpath(exe)]
    for base in {os.path.dirname(exe), os.path.dirname(os.path.realpath(exe))}:
        testembed = os.path.join(base, 'Programs', '_testembed')
        paths += [testembed, os.path.realpath(testembed)]
    return list(dict.fromkeys(path for path in paths if path))


def _text(item) -> str:
    return os.fsdecode(item)


@dataclass
class _Launch:
    shape: str
    executable: str | None
    argv: list[str] | None
    command: str | None
    env: dict[str, str]


def _launch(event: str, args: tuple) -> _Launch | None:
    """Decode an audit event without changing argv boundaries."""
    command = None
    match event:
        case 'subprocess.Popen':
            executable, argv, _cwd, env = args
        case 'os.posix_spawn' | 'os.exec':
            executable, argv, env = args
        case 'os.spawn':
            _mode, executable, argv, env = args
        case 'os.system':
            command_arg = args[0]
            command = (_text(command_arg)
                       if type(command_arg) in (str, bytes)
                       else 'opaque shell command shape')
            return _Launch('opaque-shell', None, None, command, {})
        case _:
            return None

    if type(argv) in (str, bytes):
        # Some platforms expose command strings directly. Never parse them as
        # argv: a shell command does not identify the process it may start.
        executable_text = (_text(executable)
                           if type(executable) in (str, bytes) else None)
        return _Launch('opaque-shell', executable_text, None, _text(argv), {})
    if type(argv) not in (list, tuple) or any(
            type(arg) not in (str, bytes) for arg in argv):
        return _Launch('opaque-shell', None, None,
                       'opaque launch argument shape', {})
    argv = [_text(arg) for arg in argv]
    if executable is None:
        executable = argv[0] if argv else None
    if executable is not None and type(executable) not in (str, bytes):
        return _Launch('structured', None, argv,
                       'opaque executable argument shape', {})
    executable = _text(executable) if executable is not None else None
    if env is None:
        env = dict(os.environ)
    elif type(env) is dict:
        if any(type(key) not in (str, bytes) or
               type(value) not in (str, bytes) for key, value in env.items()):
            return _Launch('structured', None, argv,
                           'opaque environment values', {})
        env = dict(env)
    else:
        return _Launch('structured', None, argv,
                       'opaque environment mapping', {})
    env = {_text(key): _text(value) for key, value in env.items()}
    return _Launch('structured', executable, argv, command, env)


_SHELL_NAMES = frozenset({
    'sh', 'bash', 'dash', 'zsh', 'ksh', 'cmd.exe', 'powershell.exe', 'pwsh.exe',
})


def _classify_launch(launch: _Launch, products: list[str]):
    """Return (target argv, environment, opaque reason), if a product starts."""
    executable = launch.executable
    if launch.shape != 'structured' or executable is None:
        return None, launch.env, 'opaque shell command'

    if os.path.isabs(executable):
        executable_path = os.path.realpath(executable)
        if executable in products or executable_path in products:
            return launch.argv, launch.env, None
    else:
        # PATH and cwd can make any relative name an alias of this build.
        # Until child-image receipts exist, do not silently claim otherwise.
        return None, launch.env, 'ambiguous relative executable'

    if os.path.basename(executable).lower() in _SHELL_NAMES:
        return None, launch.env, 'opaque shell command'

    if os.path.basename(executable) != 'env' or not launch.argv:
        return None, launch.env, None

    # Parse only the small, known env prefix grammar. Do not search arbitrary
    # argv elements for a build product.
    env = dict(launch.env)
    argv = launch.argv
    index = 1  # argv[0] is env's program name
    while index < len(argv):
        word = argv[index]
        if word in ('-i', '--ignore-environment'):
            env = {}
            index += 1
        elif '=' in word and not word.startswith('-'):
            name, _, value = word.partition('=')
            env[name] = value
            index += 1
        elif word.startswith('-'):
            return None, launch.env, 'unsupported env option'
        else:
            break
    if index >= len(argv):
        return None, launch.env, 'env command has no executable'
    target = argv[index:]
    if not os.path.isabs(target[0]):
        return None, launch.env, 'ambiguous relative env target'
    target_path = os.path.realpath(target[0])
    if target[0] not in products and target_path not in products:
        return None, launch.env, 'unresolved env target'
    return target, env, None


def _predict(argv: list[str], env: dict[str, str]):
    """(counts, reason) for an already identified interpreter argv."""
    if not argv:
        return False, 'empty interpreter argv'
    options = []
    args = iter(argv[1:])
    for arg in args:
        if not arg.startswith('-') or arg == '-':
            break
        options.append(arg)
        if arg in _ARG_OPTIONS:
            options.append(next(args, ''))
        elif arg in ('-c', '-m'):
            break
    xoptions = []
    ignore_env = False
    for i, opt in enumerate(options):
        if opt == '-X' and i + 1 < len(options):
            xoptions.append(options[i + 1])
        elif opt.startswith('-X') and len(opt) > 2:
            xoptions.append(opt[2:])
        elif opt.startswith('-') and not opt.startswith('--') and (
                'E' in opt or 'I' in opt) and opt[1] not in 'XWcm':
            ignore_env = True
    stats_dir = None
    for xopt in xoptions:
        if xopt.startswith('pystats_dir='):
            stats_dir = xopt.partition('=')[2]
    if 'pystats=0' in xoptions:
        return False, '-X pystats=0'
    if 'pystats' not in xoptions and not any(
            x.startswith('pystats=') and x != 'pystats=0' for x in xoptions):
        if ignore_env:
            return False, 'no -X pystats and -E or -I ignores PYTHONSTATS'
        if not env.get('PYTHONSTATS'):
            return False, 'no -X pystats and PYTHONSTATS unset or environment dropped'
    if not stats_dir and not env.get('PYTHON_PYSTATS_DIR'):
        return False, 'no dump directory (no -X pystats_dir or PYTHON_PYSTATS_DIR)'
    return True, ''


def _write_record(path: str, record: dict[str, Any]) -> None:
    line = (json.dumps(record) + '\n').encode()
    fd = os.open(path, os.O_WRONLY | os.O_APPEND | os.O_CREAT, 0o644)
    try:
        if os.write(fd, line) != len(line):
            raise OSError("short write while recording pystats launch")
    finally:
        os.close(fd)


def _make_spawn_hook(stats_dir: str):
    products = _build_products()
    pid_of = os.getpid
    failure_state: dict[str, Any] = {'failed': False, 'errors': [], 'omitted': 0}

    def remember_failure(event, stage, exc):
        # Set the fail-closed bit before allocating diagnostics or touching disk.
        failure_state['failed'] = True
        try:
            errors = failure_state['errors']
            if len(errors) < 16:
                errors.append({'event': event, 'stage': stage,
                               'error_type': type(exc).__name__})
            else:
                failure_state['omitted'] += 1
        except BaseException:
            # The failure bit is the allocation-free fail-closed signal.
            pass

    def append_audit_error(event, stage, exc):
        remember_failure(event, stage, exc)
        try:
            tag = os.environ.get('PYTHON_PYSTATS_TAG', '')
            name = re.sub(r'[^A-Za-z0-9_.-]', '_', tag) or 'untagged'
            _write_record(os.path.join(stats_dir, name + '.audit-error'), {
                'tag': tag, 'pid': pid_of(), 'event': event,
                'stage': stage, 'error_type': type(exc).__name__,
            })
        except BaseException as ledger_exc:
            remember_failure(event, 'error-write', ledger_exc)

    def hook(event, args):
        if event not in _SPAWN_EVENTS:
            return
        try:
            launch = _launch(event, args)
            if launch is None:
                return
            target_argv, child_env, opaque_reason = _classify_launch(launch, products)
            if target_argv is None and opaque_reason is None:
                return
            if target_argv is None:
                counts, reason = None, opaque_reason
            else:
                counts, reason = _predict(target_argv, child_env)
            tag = os.environ.get('PYTHON_PYSTATS_TAG', '')
            record = {
                'tag': tag, 'pid': pid_of(), 'event': event,
                'shape': launch.shape, 'executable': launch.executable,
                'classification': 'opaque-shell' if target_argv is None else 'direct',
                'argv': launch.argv, 'command': launch.command,
                'counts': counts, 'reason': reason,
            }
            name = re.sub(r'[^A-Za-z0-9_.-]', '_', tag) or 'untagged'
            try:
                _write_record(os.path.join(stats_dir, name + '.spawn'), record)
            except BaseException as exc:
                append_audit_error(event, 'spawn-write', exc)
        except BaseException as exc:
            append_audit_error(event, 'capture', exc)

    return hook, failure_state


def _apply_audit_failures(result, failure_state):
    if failure_state is None or not failure_state['failed']:
        return
    details = failure_state['errors']
    if failure_state['omitted']:
        details = [*details, {'stage': 'omitted',
                             'count': failure_state['omitted']}]
    if not details:
        details = [{'stage': 'diagnostics-unavailable'}]
    message = 'pystats audit capture failed: ' + json.dumps(details, sort_keys=True)
    if result.errors is None:
        result.errors = []
    result.errors.append(('<pystats audit hook>', message))
    result.state = State.FAILED


def _setup_pystats(test_name: str):
    """Tag this worker's stats and record its launches of build products."""
    if not hasattr(sys, '_stats_on'):
        return None
    # Children inherit the tag.
    os.environ['PYTHON_PYSTATS_TAG'] = test_name
    stats_dir = _stats_dir()
    if stats_dir is not None:
        hook, failure_state = _make_spawn_hook(stats_dir)
        sys.addaudithook(hook)
        return failure_state
    return None


def create_worker_process(runtests: WorkerRunTests, output_fd: int,
                          tmp_dir: StrPath | None = None) -> subprocess.Popen[str]:
    worker_json = runtests.as_json()

    cmd = runtests.create_python_cmd()
    cmd.extend(['-m', 'test.libregrtest.worker', worker_json])

    env = dict(os.environ)
    if tmp_dir is not None:
        env['TMPDIR'] = tmp_dir
        env['TEMP'] = tmp_dir
        env['TMP'] = tmp_dir

    # The worker's own dump carries the test name too (_setup_pystats does
    # the same for its children). Read at stats start, so set it here.
    if hasattr(sys, '_stats_on'):
        env['PYTHON_PYSTATS_TAG'] = runtests.tests[0]

    # The subcommand is run with a temporary output which means it is not a TTY
    # and won't auto-color. The test results are printed to stdout so if we can
    # color that have the subprocess use color.
    if can_colorize(file=sys.stdout):
        env['FORCE_COLOR'] = '1'

    # Running the child from the same working directory as regrtest's original
    # invocation ensures that TEMPDIR for the child is the same when
    # sysconfig.is_python_build() is true. See issue 15300.
    #
    # Emscripten and WASI Python must start in the Python source code directory
    # to get 'python.js' or 'python.wasm' file. Then worker_process() changes
    # to a temporary directory created to run tests.
    work_dir = os_helper.SAVEDCWD

    kwargs: dict[str, Any] = dict(
        env=env,
        stdout=output_fd,
        # bpo-45410: Write stderr into stdout to keep messages order
        stderr=output_fd,
        text=True,
        close_fds=True,
        cwd=work_dir,
    )

    # Don't use setsid() in tests using TTY
    test_name = runtests.tests[0]
    if USE_PROCESS_GROUP and test_name not in NEED_TTY:
        kwargs['start_new_session'] = True

    # Include the test name in the TSAN log file name
    if 'TSAN_OPTIONS' in env:
        parts = env['TSAN_OPTIONS'].split(' ')
        for i, part in enumerate(parts):
            if part.startswith('log_path='):
                parts[i] = f'{part}.{test_name}'
                break
        env['TSAN_OPTIONS'] = ' '.join(parts)

    # Pass json_file to the worker process
    json_file = runtests.json_file
    json_file.configure_subprocess(kwargs)

    with json_file.inherit_subprocess():
        return subprocess.Popen(cmd, **kwargs)


def worker_process(worker_json: StrJSON) -> NoReturn:
    runtests = WorkerRunTests.from_json(worker_json)
    test_name = runtests.tests[0]
    match_tests: TestFilter = runtests.match_tests
    json_file: JsonFile = runtests.json_file

    setup_test_dir(runtests.test_dir)
    setup_process()
    audit_failure_state = _setup_pystats(test_name)

    if runtests.rerun:
        if match_tests:
            matching = "matching: " + ", ".join(pattern for pattern, result in match_tests if result)
            print(f"Re-running {test_name} in verbose mode ({matching})", flush=True)
        else:
            print(f"Re-running {test_name} in verbose mode", flush=True)

    result = run_single_test(test_name, runtests)
    _apply_audit_failures(result, audit_failure_state)
    if runtests.coverage:
        if "test.cov" in sys.modules:  # imported by -Xpresite=
            result.covered_lines = list(sys.modules["test.cov"].coverage)
        elif not Py_DEBUG:
            print(
                "Gathering coverage in worker processes requires --with-pydebug",
                flush=True,
            )
        else:
            raise LookupError(
                "`test.cov` not found in sys.modules but coverage wanted"
            )

    if json_file.file_type == JsonFileType.STDOUT:
        print()
        result.write_json_into(sys.stdout)
    else:
        with json_file.open('w', encoding='utf-8') as json_fp:
            result.write_json_into(json_fp)

    sys.exit(0)


def main() -> NoReturn:
    if len(sys.argv) != 2:
        print("usage: python -m test.libregrtest.worker JSON")
        sys.exit(1)
    worker_json = sys.argv[1]

    tmp_dir = get_temp_dir()
    # get_temp_dir() can be different in the worker and the parent process.
    # For example, if --tempdir option is used.
    os.makedirs(tmp_dir, exist_ok=True)
    work_dir = get_work_dir(tmp_dir, worker=True)

    with exit_timeout():
        with os_helper.temp_cwd(work_dir, quiet=True):
            worker_process(worker_json)


if __name__ == "__main__":
    main()
