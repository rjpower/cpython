import ast
import subprocess
import sys
from test import test_gc

control = sys.argv[1]
if control == "forwarding":
    source = subprocess.check_output(
        ["git", "show", "0f8a019b846:Lib/test/test_gc.py"], text=True)
    fn = next(node for node in ast.parse(source).body
              if isinstance(node, ast.FunctionDef) and node.name == "_queue_child")
    exec(compile(ast.Module(body=[fn], type_ignores=[]),
                 "<unfixed helper at 0f8a019b846>", "exec"), test_gc.__dict__)
    test_gc._queue_child("assert _testinternalcapi.gc_verify_mode() == (2, 0), "
                         "_testinternalcapi.gc_verify_mode()")
    print("old helper unexpectedly preserved kill")
    raise SystemExit(0)

original = test_gc._queue_child

def changed_child(code, **env):
    if control == "duplicate_pin":
        old = 'log.append((kind, token, where()))'
        new = 'log.append((kind, 0 if token == 1 else token, where()))'
    elif control == "duplicate_alloc":
        old = ':{{token}} '
        new = ':{{token - 1 if token % 10 == 1 else token}} '
    elif control == "no_ready":
        old = '            t = threading.Thread(target=collector)'
        new = ('            os.environ["BOIL_QUEUE_CONTROL"] = "no_ready"\n' + old)
    elif control == "no_roots":
        old = '            lengths = _testinternalcapi.gc_collect_inline_then(second, 1)'
        new = ('            os.environ["BOIL_QUEUE_CONTROL"] = "no_roots"\n' + old)
    else:
        raise ValueError(control)
    assert code.count(old) == 1, (control, code.count(old))
    return original(code.replace(old, new), **env)

test_gc._queue_child = changed_child
method = {"duplicate_pin": "test_pin", "duplicate_alloc": "test_allocation_failure",
          "no_ready": "test_ready_for_every_thread", "no_roots": "test_queued_payload_survives_collection"}[control]
test_gc.InterpreterQueueTests(method).__getattribute__(method)()
print("negative control unexpectedly passed")
