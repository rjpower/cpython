import json
import re
import shlex
import subprocess
import sys
import os
from _colorize import can_colorize  # type: ignore[import-not-found]
from typing import Any, NoReturn

from test.support import os_helper, Py_DEBUG

from .setup import setup_process, setup_test_dir
from .runtests import WorkerRunTests, JsonFile, JsonFileType
from .single import run_single_test
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


def _find_product(tokens: list[str], products: list[str]) -> int | None:
    for i, token in enumerate(tokens):
        if token in products:
            return i
    return None


def _predict(argv: list[str], env: dict[str, str], products: list[str]):
    """(counts, reason) for the child that argv starts, given its environment."""
    start = _find_product(argv, products)
    # Words before the program: "env -i", NAME=value.
    env = dict(env)
    for word in argv[:start]:
        if word == '-i' or word == '--ignore-environment':
            env = {}
        elif '=' in word and not word.startswith('-'):
            name, _, value = word.partition('=')
            env[name] = value
    options = []
    args = iter(argv[start + 1:])
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


def _launch(event: str, args: tuple):
    """(argv, env) of the launch, or None for a shape the hook ignores."""
    match event:
        case 'subprocess.Popen':
            _executable, argv, _cwd, env = args
        case 'os.posix_spawn' | 'os.exec':
            _path, argv, env = args
        case 'os.spawn':
            _mode, _path, argv, env = args
        case 'os.system':
            argv, env = args[0], None
        case _:
            return None
    if isinstance(argv, (str, bytes, os.PathLike)):
        # A command string (os.system, a Popen with a string): the shell
        # words, so that a product is found inside the string.
        try:
            argv = shlex.split(_text(argv))
        except ValueError:
            argv = _text(argv).split()
    argv = [_text(arg) for arg in argv]
    # A shell command string inside argv ("sh -c 'python ...'").
    words = []
    for arg in argv:
        if ' ' in arg:
            try:
                words.extend(shlex.split(arg))
                continue
            except ValueError:
                words.extend(arg.split())
                continue
        words.append(arg)
    if env is None:
        env = os.environ
    env = {_text(k): _text(v) for k, v in dict(env).items()}
    return words, env


def _make_spawn_hook(stats_dir: str):
    products = _build_products()
    pid_of = os.getpid

    def hook(event, args):
        if event not in _SPAWN_EVENTS:
            return
        # The hook must never raise into the test.
        try:
            launch = _launch(event, args)
            if launch is None:
                return
            words, env = launch
            if _find_product(words, products) is None:
                return
            counts, reason = _predict(words, env, products)
            tag = os.environ.get('PYTHON_PYSTATS_TAG', '')
            line = json.dumps({
                'tag': tag, 'pid': pid_of(), 'event': event,
                'argv': words, 'counts': counts, 'reason': reason,
            }) + '\n'
            name = re.sub(r'[^A-Za-z0-9_.-]', '_', tag) or 'untagged'
            fd = os.open(os.path.join(stats_dir, name + '.spawn'),
                         os.O_WRONLY | os.O_APPEND | os.O_CREAT, 0o644)
            try:
                os.write(fd, line.encode())
            finally:
                os.close(fd)
        except Exception:
            pass

    return hook


def _setup_pystats(test_name: str) -> None:
    """Tag this worker's stats and record its launches of build products."""
    if not hasattr(sys, '_stats_on'):
        return
    # Children inherit the tag.
    os.environ['PYTHON_PYSTATS_TAG'] = test_name
    stats_dir = _stats_dir()
    if stats_dir is not None:
        sys.addaudithook(_make_spawn_hook(stats_dir))


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
    _setup_pystats(test_name)

    if runtests.rerun:
        if match_tests:
            matching = "matching: " + ", ".join(pattern for pattern, result in match_tests if result)
            print(f"Re-running {test_name} in verbose mode ({matching})", flush=True)
        else:
            print(f"Re-running {test_name} in verbose mode", flush=True)

    result = run_single_test(test_name, runtests)
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
