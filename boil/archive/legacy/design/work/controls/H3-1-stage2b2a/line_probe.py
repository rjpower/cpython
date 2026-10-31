"""Use the real test program and monitoring cache for copied line helpers."""

import hashlib
import importlib.util
import json
import sys
import textwrap

import _testinternalcapi
from test import test_code


def main():
    extension, name = sys.argv[1:]
    spec = importlib.util.spec_from_file_location("_h3_safe_line", extension)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    helper = test_code.CodeLocationTest.check_safe_addr2line
    source = next(value for value in helper.__code__.co_consts
                  if isinstance(value, str) and "def loop():" in value)
    source = textwrap.dedent(source)
    previous = _testinternalcapi.set_ir_max_forward(0)
    try:
        namespace = {}
        exec(compile(source, "<safe-line-lookup>", "exec"), namespace)
    finally:
        _testinternalcapi.set_ir_max_forward(previous)
    func = namespace[name]
    code = func.__code__
    handlers = [word[0] for word in _testinternalcapi.get_ir(code)]
    expected = {index: -1 if line is None else line
                for start, end, line in code.co_lines()
                for index in range(start, end)}
    jumps = [i for i, handler in enumerate(handlers) if handler == "IR_JUMP"]
    assert len(jumps) == (3 if name == "loop" else 2)
    assert all(expected[i] >= 0 for i in jumps)
    for index, line in expected.items():
        assert module.lookup(code, index) == line
        assert _testinternalcapi.code_safe_addr2line(code, index) == line
    mon = sys.monitoring
    tool = mon.PROFILER_ID
    mon.use_tool_id(tool, "copied line lookup")
    try:
        mon.set_local_events(tool, code, mon.events.LINE | mon.events.INSTRUCTION)
        result = {"function": name, "co_code_sha256": hashlib.sha256(code.co_code).hexdigest(),
                  "linetable_sha256": hashlib.sha256(code.co_linetable).hexdigest(),
                  "source_sha256": hashlib.sha256(source.encode()).hexdigest(),
                  "rows": [[i, handlers[i], line,
                            _testinternalcapi.code_safe_addr2line(code, i),
                            module.lookup(code, i)] for i, line in expected.items()]}
        print(json.dumps(result), flush=True)
        for index, handler, line, live, copied in result["rows"]:
            assert live == line
            if handler != "IR_JUMP":
                assert copied == line, ("real instruction head", index)
        for index in jumps:
            assert module.lookup(code, index) == expected[index], (
                "immutable synthetic line", name, index,
                module.lookup(code, index), expected[index])
    finally:
        mon.set_local_events(tool, code, 0)
        mon.free_tool_id(tool)


if __name__ == "__main__":
    main()
