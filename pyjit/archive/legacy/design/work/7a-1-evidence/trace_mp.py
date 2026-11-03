import gc
import runpy
import sys
import threading
import traceback
import _testinternalcapi
from test import _test_multiprocessing as mp

original_teardown = mp.BaseMixin.tearDownClass.__func__
original_warning = mp.support.print_warning

def traced_teardown(cls):
    def warning(message):
        if message.startswith("Dangling threads:"):
            threads = set(threading._dangling) - set(cls.dangling[1])
            print("FEEDER_TRACE class", cls.__name__, "mode",
                  _testinternalcapi.gc_verify_mode(), file=sys.stderr, flush=True)
            frames = sys._current_frames()
            for thread in threads:
                ident = thread.ident
                print("FEEDER_STATE", thread.name, "alive", thread.is_alive(),
                      "active", threading._active.get(ident) is thread,
                      "limbo", thread in threading._limbo,
                      "attributes", {key: type(value).__name__
                                     for key, value in vars(thread).items()},
                      file=sys.stderr, flush=True)
                frame = frames.get(ident)
                if frame is not None:
                    print("FEEDER_STACK", file=sys.stderr)
                    traceback.print_stack(frame, file=sys.stderr)
                hits, dropped = _testinternalcapi.gc_stack_hits(id(thread))
                print("FEEDER_STACK_HITS", hits, "dropped", dropped,
                      file=sys.stderr, flush=True)
                refs = gc.get_referrers(thread)
                for ref in refs:
                    keys = ([str(key) for key, value in ref.items() if value is thread]
                            if type(ref) is dict else [])
                    print("FEEDER_REFERRER", type(ref).__name__, keys,
                          file=sys.stderr, flush=True)
            import multiprocessing.util as util
            for key, finalizer in list(util._finalizer_registry.items()):
                callback = finalizer._callback
                print("FEEDER_FINALIZER", key,
                      callback.__qualname__ if callback is not None else None,
                      [type(arg).__name__ for arg in finalizer._args],
                      file=sys.stderr, flush=True)
        original_warning(message)
    warning.orig_stderr = original_warning.orig_stderr
    mp.support.print_warning = warning
    try:
        original_teardown(cls)
    finally:
        mp.support.print_warning = original_warning

mp.BaseMixin.tearDownClass = classmethod(traced_teardown)
args = sys.argv[1:]
if args and args[0] == "-u":
    args = args[1:]
if len(args) < 2 or args[0] != "-m":
    raise SystemExit("expected a regrtest worker module")
sys.argv = [args[1], *args[2:]]
runpy.run_module(args[1], run_name="__main__", alter_sys=True)
