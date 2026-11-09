"""Coordinator-only bounded phase inventory of four still-failing cases."""
import ast
import dis
import importlib.util
import inspect
import json
from pathlib import Path
import sys
import textwrap
import types
import warnings

from _testinternalcapi import get_ir
from test import test_monitoring, test_peepholer


def describer():
    path = Path(__file__).with_name('probe-debugger-context-inventory.py')
    spec = importlib.util.spec_from_file_location('_remaining_inventory', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.describe


def snapshot(frame):
    pc = frame.f_lasti
    return dict(pc=pc, line=frame.f_lineno,
                actual_word=get_ir(frame.f_code, True)[pc],
                bound_locals=sorted(frame.f_locals))


def request(frame, target):
    row = dict(before=snapshot(frame), target=target)
    try:
        frame.f_lineno = target
    except ValueError as error:
        row['refusal'] = str(error)
    else:
        row['committed'] = True
    row['after'] = snapshot(frame)
    return row


def main():
    describe = describer()
    rows = {}
    for name in ('test_setting_lineno_no_undefined',
                 'test_setting_lineno_one_undefined',
                 'test_setting_lineno_two_undefined'):
        method = getattr(test_peepholer.TestMarkingVariablesAsUnKnown, name)
        tree = ast.parse(textwrap.dedent(inspect.getsource(method)))
        text = next(node.args[0].value for node in ast.walk(tree)
                    if isinstance(node, ast.Call) and
                    isinstance(node.func, ast.Attribute) and
                    isinstance(node.func.value, ast.Name) and
                    node.func.value.id == 'textwrap' and node.func.attr == 'dedent')
        source = textwrap.dedent(text)
        namespace = {}
        exec(compile(source, '<remaining:' + name + '>', 'exec'), namespace)
        function = namespace['f']
        attempts = []
        count = 0
        def trace(frame, event, arg):
            nonlocal count
            if frame.f_code is function.__code__ and event == 'line':
                count += 1
                if count > 250:
                    raise RuntimeError('remaining phase probe event cap')
                if frame.f_lineno == 9 and not attempts:
                    attempts.append(request(frame, 3))
                    sys.settrace(None)
            return trace
        previous = sys.gettrace()
        with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter('always', RuntimeWarning)
            sys.settrace(trace)
            try:
                result = function()
            finally:
                sys.settrace(previous)
        rows[name] = dict(source=source, code=describe(function.__code__),
                          attempts=attempts, result=result,
                          warnings=[str(item.message) for item in caught])

    method = test_monitoring.TestBranchAndJumpEvents.test_callback_set_frame_lineno
    code = next(value for value in method.__code__.co_consts
                if isinstance(value, types.CodeType) and value.co_name == 'func')
    function = types.FunctionType(code, method.__globals__)
    attempts = []
    mon, tool = sys.monitoring, 4
    def callback(code, source, destination):
        if code is function.__code__:
            frame = sys._getframe(1)
            attempts.append(dict(source=source, destination=destination,
                                 request=request(frame, frame.f_lineno + 1)))
    mon.use_tool_id(tool, 'remaining-phase-probe')
    try:
        mon.register_callback(tool, mon.events.BRANCH_LEFT, callback)
        mon.set_local_events(tool, code, mon.events.BRANCH_LEFT)
        result = function('true')
    finally:
        mon.set_local_events(tool, code, 0)
        mon.register_callback(tool, mon.events.BRANCH_LEFT, None)
        mon.free_tool_id(tool)
    rows['monitoring_branch'] = dict(code=describe(code), attempts=attempts, result=result)
    print(json.dumps(dict(python=sys.version, rows=rows), indent=2))


if __name__ == '__main__':
    main()
