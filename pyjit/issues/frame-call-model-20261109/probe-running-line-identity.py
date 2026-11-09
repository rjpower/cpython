"""Coordinator-run capped reproduction of the unchanged running-line test."""

import dis
import json
import sys
import warnings

from test.test_sys_settrace import JumpFrameModelTestCase


class EventLimit(BaseException):
    pass


def instructions(code, adaptive):
    return [
        {
            "pc": item.offset,
            "opname": item.opname,
            "arg": item.arg,
            "line": item.positions.lineno,
            "end_line": item.positions.end_lineno,
            "column": item.positions.col_offset,
            "end_column": item.positions.end_col_offset,
        }
        for item in dis.get_instructions(code, adaptive=adaptive)
    ]


def probe(name, source, warm):
    namespace = {}
    exec(compile(source, f"<running-line:{name}>", "exec"), namespace)
    function = namespace["f"]
    code = function.__code__
    value = JumpFrameModelTestCase.Value()
    args = (value,) * code.co_argcount
    expected = function(*args)
    for _ in range(warm):
        function(*args)
    events = []
    capped = False
    error = None
    actual_is_expected = False

    def tracer(frame, event, arg):
        if frame.f_code is not code:
            return None
        if event == "line":
            if len(events) == 80:
                raise EventLimit
            row = {"before_pc": frame.f_lasti, "line": frame.f_lineno}
            events.append(row)
            try:
                frame.f_lineno = frame.f_lineno
            except ValueError as exc:
                row["refusal"] = str(exc)
            row["after_pc"] = frame.f_lasti
            row["after_line"] = frame.f_lineno
        return tracer

    static_words = instructions(code, False)
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", RuntimeWarning)
        sys.settrace(tracer)
        try:
            actual_is_expected = function(*args) is expected
        except EventLimit:
            capped = True
        except Exception as exc:
            error = {"type": type(exc).__name__, "message": str(exc)}
        finally:
            sys.settrace(None)
    return {
        "name": name,
        "warm": warm,
        "source": source,
        "capped": capped,
        "error": error,
        "actual_is_expected": actual_is_expected,
        "events": events,
        "static_words": static_words,
        "observed_words": instructions(code, True),
        "co_lines": list(code.co_lines()),
    }


names = set(sys.argv[1:])
rows = [
    probe(name, source, warm)
    for name, source in JumpFrameModelTestCase.SOURCES.items()
    if not names or name in names
    for warm in (0, 20)
]
print(json.dumps(rows, indent=2))
