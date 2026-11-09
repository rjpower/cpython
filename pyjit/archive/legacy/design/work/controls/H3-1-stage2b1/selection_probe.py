"""Compare copied selection with the same real monitored NOFB word."""

import dis
import gc
import importlib.util
import json
import sys

import _testinternalcapi
from test.support.bytecode_helper import encode_words


def probe(extension):
    class A:
        value = 7

    class B(A):
        def f(self):
            return super().value

    f = B.f
    item = next(i for i in dis.get_instructions(f)
                if i.opname == "LOAD_SUPER_ATTR")
    _, a, b, _, _ = _testinternalcapi.get_ir(f.__code__)[item.offset]
    handler = dis._all_opmap["LOAD_SUPER_ATTR_NOFB"]
    word = (handler, a, b, item.arg & 4095, item.arg >> 12)
    raw = f.__code__.co_code
    pristine = encode_words(word)
    f.__code__ = f.__code__.replace(co_code=(
        raw[:item.offset * 8] + pristine + raw[(item.offset + 1) * 8:]))
    assert f(B()) == 7
    get_word = lambda: int.from_bytes(f.__code__._co_code_adaptive[
        item.offset * 8:(item.offset + 1) * 8], "little")
    before = int.from_bytes(pristine, "little")
    assert get_word() == before
    mon = sys.monitoring
    tool = mon.PROFILER_ID
    mon.use_tool_id(tool, "selection control")
    events = []

    def callback(event):
        def collect(code, offset, callable, arg0):
            if code is f.__code__ and offset == item.offset:
                events.append([event, get_word()])
                gc.collect()
        return collect

    try:
        for event in (mon.events.CALL, mon.events.C_RETURN, mon.events.C_RAISE):
            mon.register_callback(tool, event, callback(event))
        mon.set_local_events(tool, f.__code__, mon.events.CALL)
        installed = get_word()
        assert installed & 65535 == dis._all_opmap[
            "INSTRUMENTED_LOAD_SUPER_ATTR_NOFB"]
        assert installed >> 16 == before >> 16
        assert f(B()) == 7
        assert [mon.events.CALL, installed] in events
        assert [mon.events.C_RETURN, installed] in events
        try:
            f(object())
        except TypeError:
            pass
        else:
            raise AssertionError("super accepted unrelated self")
        assert [mon.events.C_RAISE, installed] in events
        mon.set_local_events(tool, f.__code__, 0)
        restored = get_word()
        assert restored == before
        assert f(B()) == 7
        spec = importlib.util.spec_from_file_location("_h3_selection", extension)
        copied = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(copied)
        selected = copied.select(f.__code__, item.offset,
                                dis._all_opmap["INSTRUMENTED_LOAD_SUPER_ATTR"],
                                dis._all_opmap["LOAD_SUPER_ATTR"])
        result = {"before": before, "live_installed": installed,
                  "live_restored": restored, "copied": list(selected),
                  "extension": copied.__file__, "events": events}
        print(json.dumps(result), flush=True)
        assert selected[0] == installed, ("installed NOFB twin", result)
        assert selected[1] == before, ("exact pristine restoration", result)
    finally:
        mon.set_local_events(tool, f.__code__, 0)
        for event in (mon.events.CALL, mon.events.C_RETURN, mon.events.C_RAISE):
            mon.register_callback(tool, event, None)
        mon.free_tool_id(tool)


if __name__ == "__main__":
    probe(sys.argv[1])
