"""Repeat the profile regression; gather expensive state only after failure.

Run with a qualified runtime, for example: python probe-profile-events.py
--rounds 100 --output evidence.json. This is a diagnostic, not a timing test.
"""

import argparse
import dis
import io
import json
from pathlib import Path
import pstats
import sys
import unittest

from test.test_profile import profile, timer


STATEMENTS = (
    "max([0])",
    "max([0], key=int)",
    "max([0], **dict(key=int))",
    "max(*([0],))",
    "max(*([0],), key=int)",
    "max(*([0],), **dict(key=int))",
)


def run_profile(command, record=False):
    prof = profile.Profile(timer, 0.001)
    events = []
    if record:
        dispatcher = prof.dispatcher

        def recording_dispatcher(frame, event, arg):
            # Keep code objects, not frames. Do introspection after profiling.
            if len(events) < 1000:
                name = arg.__name__ if event in (
                    "c_call", "c_return", "c_exception") else None
                events.append((frame.f_code, frame.f_lasti, event, name))
            return dispatcher(frame, event, arg)

        prof.dispatcher = recording_dispatcher
    prof.runctx(command, globals(), locals())
    output = io.StringIO()
    stats = pstats.Stats(prof, stream=output)
    stats.print_stats()
    return output.getvalue(), events


def code_state(code):
    import _testinternalcapi

    return {
        "name": code.co_qualname,
        "file": code.co_filename,
        "line": code.co_firstlineno,
        "bytecode": dis.Bytecode(code, adaptive=True).dis(),
        "canonical_ir": _testinternalcapi.get_ir(code),
        "physical_ir": _testinternalcapi.get_ir(code, True),
        "local_events": [sys.monitoring.get_local_events(i, code)
                         for i in range(6)],
    }


def replay(statement, precompiled):
    command = (compile(statement, "<profile-events-probe>", "exec")
               if precompiled else statement)
    stats, raw = run_profile(command, record=True)
    codes = []
    if precompiled:
        codes.append(command)
    events = []
    for code, offset, event, name in raw:
        index = next((i for i, known in enumerate(codes) if known is code), None)
        if index is None:
            index = len(codes)
            codes.append(code)
        events.append({"code": index, "offset": offset,
                       "event": event, "callable": name})
    return {
        "mode": "precompiled" if precompiled else "string",
        "has_max": ":0(max)" in stats,
        "stats": stats,
        "events": events,
        "event_limit_reached": len(raw) == 1000,
        "codes": [code_state(code) for code in codes],
    }


def actual_fixtures(args, result):
    from test.test_profile import ProfileTest
    from test.test_profiling.test_tracing_profiler import CProfileTest

    method = ProfileTest.test_calling_conventions
    assert CProfileTest.test_calling_conventions is method
    classes = (CProfileTest, ProfileTest)
    if args.order == "p-first":
        classes = tuple(reversed(classes))
    result.update({"mode": "actual-fixture", "order": args.order,
                   "shared_method": True})
    for iteration in range(args.rounds):
        for cls in classes:
            case = cls("test_calling_conventions")
            outcome = unittest.TestResult()
            case.run(outcome)
            result["completed"] += outcome.testsRun
            if outcome.wasSuccessful():
                continue
            result.update({
                "passed": False, "iteration": iteration,
                "fixture": cls.__name__,
                "failures": [message for _, message in outcome.failures],
                "errors": [message for _, message in outcome.errors],
                # Snapshot before diagnostic replays alter this shared method.
                "fixture_method": code_state(method.__code__),
                "python_runctx": code_state(profile.Profile.runctx.__code__),
            })
            return


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rounds", type=int, default=100)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--actual-fixture", action="store_true",
                        help="Run the inherited unittest method with both profilers")
    parser.add_argument("--order", choices=("c-first", "p-first"), default="c-first")
    args = parser.parse_args()
    assert args.rounds > 0
    assert sys.getprofile() is None, "Run in a fresh process without a profiler"
    result = {"executable": sys.executable, "version": sys.version,
              "rounds": args.rounds, "completed": 0, "passed": True}
    if args.actual_fixture:
        actual_fixtures(args, result)
    for iteration in range(0 if args.actual_fixture else args.rounds):
        for statement in STATEMENTS:
            stats, _ = run_profile(statement)
            result["completed"] += 1
            if ":0(max)" in stats:
                continue
            result.update({"passed": False, "iteration": iteration,
                           "statement": statement, "original_stats": stats})
            break
        if not result["passed"]:
            break
    if not result["passed"]:
        # Public monitoring IDs exclude the private legacy-profile tool.
        result["monitoring"] = [
            {"id": i, "tool": sys.monitoring.get_tool(i),
             "events": sys.monitoring.get_events(i)} for i in range(6)]
        result["profile_left_active"] = sys.getprofile() is not None
        failure = "\n".join(result.get("failures", []) + result.get("errors", []))
        statement = result.get("statement")
        if statement is None:
            statement = next((stmt for stmt in sorted(STATEMENTS, key=len, reverse=True)
                              if stmt in failure), STATEMENTS[-1])
        result["replay_statement"] = statement
        result["replays"] = [replay(statement, False), replay(statement, True)]
    output = json.dumps(result, indent=2) + "\n"
    if args.output:
        args.output.write_text(output)
    else:
        print(output, end="")
    assert result["passed"], "Profile calling-convention failure; see diagnostic JSON"


if __name__ == "__main__":
    main()
