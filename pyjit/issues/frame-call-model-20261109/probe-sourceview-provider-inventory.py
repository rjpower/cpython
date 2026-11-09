"""Coordinator-only compile inventory of the unchanged each-LINE fixtures."""
import ast
import importlib.util
import inspect
import json
from pathlib import Path
import sys
import textwrap
import types

from test import test_sys_settrace


def main():
    path = Path(__file__).with_name('probe-debugger-context-inventory.py')
    spec = importlib.util.spec_from_file_location('_sourceview_inventory', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    method = test_sys_settrace.JumpFrameModelTestCase.test_jump_out_of_comprehension_at_each_line_event
    tree = ast.parse(textwrap.dedent(inspect.getsource(method)))
    sources = next(ast.literal_eval(node.value) for node in ast.walk(tree)
                   if isinstance(node, ast.Assign) and
                   any(isinstance(target, ast.Name) and target.id == 'sources'
                       for target in node.targets))
    cases = {}
    for label, source in sources.items():
        root = compile(source, '<sourceview:' + label + '>', 'exec')
        code = next(value for value in root.co_consts
                    if isinstance(value, types.CodeType) and value.co_name == 'f')
        description = module.describe(code)
        words = description['words']
        events = []
        for event in description['source_events']:
            events.append(dict(event, from_operation=words[event['from']]['operation']))
        description['source_events'] = events
        cases[label] = dict(source=source, code=description)
    print(json.dumps(dict(python=sys.version, compile_only=True,
                          fixture_method=method.__qualname__, cases=cases), indent=2))


if __name__ == '__main__':
    main()
