"""Read actual fixed LINE slots for valid relaxed FOR_ITER/SEND code."""
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import sysconfig
import _testinternalcapi as capi


def main():
    assert sys.flags.optimize == 0 and __debug__
    assert capi.gc_verify_mode() == (0, 0) and capi.gc_get_stress() == 0
    directory = Path(__file__).resolve().parent
    build = Path(sys.executable).resolve().parent
    extension = directory / ("allocation-" + build.name) / ("_h3_allocation" + sysconfig.get_config_var("EXT_SUFFIX"))
    manifest = json.loads(extension.with_name("manifest.json").read_text())
    assert hashlib.sha256(extension.read_bytes()).hexdigest() == manifest["extension_sha256"]
    assert all(hashlib.sha256(Path(name).read_bytes()).hexdigest() == value
               for name, value in manifest["identities_before"].items())
    spec = importlib.util.spec_from_file_location("_h3_allocation", extension)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    source = """
def loop():
    total = 0
    for value in (1, 2, 3):
        total = (
            total + value
        )
    return total

def inner():
    yield 1

def send():
    yield from inner()
"""
    previous = capi.set_ir_max_forward(0)
    try:
        namespace = {}
        exec(compile(source, "<fixed-synthetic-lines>", "exec"), namespace)
    finally:
        capi.set_ir_max_forward(previous)
    mon = sys.monitoring
    tool = mon.PROFILER_ID
    mon.use_tool_id(tool, "fixed synthetic LINE slots")
    observed = []
    def record(current, offset):
        if current is code:
            observed.append(offset)
    mon.register_callback(tool, mon.events.INSTRUCTION, record)
    try:
        for name, expected_jumps in (("loop", 3), ("send", 2)):
            func = namespace[name]
            code = func.__code__
            words = capi.get_ir(code)
            synthetic = [i for i, word in enumerate(words) if word[0] == "IR_JUMP"]
            assert len(synthetic) == expected_jumps, words
            try:
                module.inspect_lines(code)
            except ValueError:
                pass
            else:
                raise AssertionError("inspector performed lazy preparation")
            mon.set_local_events(tool, code, mon.events.LINE | mon.events.INSTRUCTION)
            cached = module.inspect_lines(code)
            immutable = [capi.code_safe_addr2line(code, i) for i in range(len(words))]
            for i, pair in enumerate(cached):
                if i in synthetic:
                    assert tuple(pair) == (-1, 0), (name, i, pair)
                    assert immutable[i] >= 0, (name, i, immutable)
                else:
                    assert pair[0] == immutable[i], (name, i, pair, immutable[i])
            observed.clear()
            assert func() == 6 if name == "loop" else list(func()) == [1]
            assert observed and not set(synthetic).intersection(observed)
            print(json.dumps({"name": name, "synthetic": synthetic, "cached": cached,
                              "immutable": immutable, "co_lines": list(code.co_lines()),
                              "instruction_sites": observed}), flush=True)
            mon.set_local_events(tool, code, 0)
    finally:
        for name in ("loop", "send"):
            mon.set_local_events(tool, namespace[name].__code__, 0)
        mon.free_tool_id(tool)


if __name__ == "__main__":
    main()
