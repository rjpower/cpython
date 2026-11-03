import ast
import subprocess
import sys
import unittest
from test import test_gc

source = subprocess.check_output(
    ["git", "show", "a0338fd7634:Lib/test/test_gc.py"], text=True)
cls = next(node for node in ast.parse(source).body
           if isinstance(node, ast.ClassDef) and node.name == "InterpreterQueueTests")
fn = next(node for node in cls.body
          if isinstance(node, ast.FunctionDef) and node.name == "test_pin")
fn.name = "original_pin"
exec(compile(ast.Module(body=[fn], type_ignores=[]), "<original a0338fd7634 pin>", "exec"),
     test_gc.__dict__)
original_child = test_gc._queue_child
original_ok = test_gc.assert_python_ok

def capture(*args, **env):
    result = original_ok(*args, **env)
    print(result[2].decode(), end="", file=sys.stderr)
    return result

def traced_child(code, **env):
    old = "            gc.collect()\n            t.join()"
    new = """            trace = []
            def trace_gc(phase, info):
                b = _testinternalcapi.gc_queue_info()["barrier"]
                trace.append((phase, [i[0] for i in _interpreters.list_all()],
                              b["remapped"], b["unpinned"]))
            gc.callbacks.append(trace_gc)
            gc.collect()
            gc.callbacks.remove(trace_gc)
            t.join()
            print("PIN_TRACE", trace, _testinternalcapi.gc_queue_info()["barrier"],
                  file=sys.stderr)"""
    assert code.count(old) == 1
    return original_child(code.replace(old, new), **env)

test_gc.assert_python_ok = capture
test_gc._queue_child = traced_child
for run in range(int(sys.argv[1]) if len(sys.argv) > 1 else 20):
    print("RUN", run, file=sys.stderr, flush=True)
    test_gc.original_pin(unittest.TestCase())
