"""Coordinator-run plain Python probe: source-only callback redirect intent."""
import json
import sys


def factory():
    def target():
        x = 1
        pass
        return x
    target.__code__ = target.__code__.replace()
    return target


def probe(warm, redirect):
    target = factory()
    for _ in range(warm):
        target()
    first = target.__code__.co_firstlineno
    events = []
    applied = False
    def trace(frame, event, arg):
        nonlocal applied
        if frame.f_code is target.__code__ and event == "line":
            events.append({"line": frame.f_lineno - first,
                           "lasti": frame.f_lasti,
                           "x": frame.f_locals.get("x")})
            if frame.f_lineno == first + 2 and not applied:
                applied = True
                frame.f_locals["x"] = 99
                if redirect:
                    before = frame.f_lasti
                    frame.f_lineno = first + 1
                    events.append({"commit_before": before,
                                   "commit_after": frame.f_lasti})
        return trace
    try:
        sys.settrace(trace)
        value = target()
    finally:
        sys.settrace(None)
    return {"warm": warm, "redirect": redirect,
            "value": value, "events": events}


print(json.dumps([probe(warm, redirect)
                  for warm in (0, 20) for redirect in (False, True)], indent=2))
