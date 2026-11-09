"""Coordinator-only bounded trace probe; never installs an unchecked mutant."""
import ast
import inspect
import json
import sys
import textwrap
import warnings

from _testinternalcapi import get_ir
from test import test_sys_settrace


def sources():
    method = test_sys_settrace.JumpFrameModelTestCase.test_jump_out_of_comprehension_at_each_line_event
    tree = ast.parse(textwrap.dedent(inspect.getsource(method)))
    return next(ast.literal_eval(node.value) for node in ast.walk(tree)
                if isinstance(node, ast.Assign) and
                any(isinstance(target, ast.Name) and target.id == 'sources'
                    for target in node.targets))


def probe(label, source, ordinal, warm):
    namespace = {}
    exec(compile(source, '<each-line-phase:' + label + '>', 'exec'), namespace)
    function = namespace['f']
    code = function.__code__
    values = [(1, 2), (3, 4), (5, 6)]
    for _ in range(warm):
        try:
            function(values)
        except TypeError:
            pass
    target = max(line for _, _, line in code.co_lines() if line)
    events = []
    attempt = None

    def tracer(frame, event, arg):
        nonlocal attempt
        if frame.f_code is not code or event != 'line':
            return tracer
        if len(events) >= 40:
            raise RuntimeError('bounded each-LINE probe event cap')
        pc = frame.f_lasti
        row = dict(pc=pc, line=frame.f_lineno,
                   actual_word=get_ir(code, True)[pc],
                   pristine_word=get_ir(code)[pc],
                   bound_locals=sorted(frame.f_locals))
        events.append(row)
        if len(events) == ordinal:
            attempt = dict(before=row, target=target)
            try:
                frame.f_lineno = target
            except ValueError as error:
                attempt['refusal'] = str(error)
            else:
                attempt['committed'] = True
            attempt['after_pc'] = frame.f_lasti
            attempt['after_line'] = frame.f_lineno
            attempt['after_bound_locals'] = sorted(frame.f_locals)
            sys.settrace(None)
        return tracer

    previous = sys.gettrace()
    with warnings.catch_warnings(record=True) as caught:
        warnings.simplefilter('always', RuntimeWarning)
        sys.settrace(tracer)
        try:
            result = function(values)
            outcome = dict(result=result)
        except TypeError as error:
            outcome = dict(type_error=str(error))
        finally:
            sys.settrace(previous)
    return dict(label=label, ordinal=ordinal, warm=warm, events=events,
                attempt=attempt, outcome=outcome,
                warnings=[str(item.message) for item in caught])


def main():
    rows = [probe(label, source, ordinal, warm)
            for label, source in sources().items()
            for warm in (0, 20) for ordinal in range(1, 8)]
    print(json.dumps(dict(python=sys.version, rows=rows), indent=2))


if __name__ == '__main__':
    main()
