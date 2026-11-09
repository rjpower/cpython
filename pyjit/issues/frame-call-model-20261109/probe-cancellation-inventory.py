"""Coordinator-only compile inventory; never invokes fixture functions."""
import ast
import importlib.util
import inspect
import json
from pathlib import Path
import sys
import textwrap
import types

from test import test_monitoring, test_peepholer, test_sys_settrace


def load_describer():
    path = Path(__file__).with_name('probe-debugger-context-inventory.py')
    spec = importlib.util.spec_from_file_location('_context_inventory', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.describe


def nested_code(code, name):
    return next(value for value in code.co_consts
                if isinstance(value, types.CodeType) and value.co_name == name)


def main():
    describe = load_describer()
    result = {}
    names = (
        'test_jump_over_discarded_call',
        'test_jump_out_of_multiline_nested_call',
        'test_jump_out_of_multiline_method_call',
        'test_jump_across_with',
        'test_jump_out_of_with_block_within_for_block',
        'test_jump_out_of_with_block_within_with_block',
        'test_jump_out_of_block_forwards',
        'test_jump_within_except_block',
        'test_jump_out_of_multiline_comprehension',
    )
    for name in names:
        wrapper = getattr(test_sys_settrace.JumpTestCase, name)
        function = inspect.unwrap(wrapper)
        closure = inspect.getclosurevars(wrapper).nonlocals
        result[name] = dict(
            source=inspect.getsource(function),
            jump_from=closure['jumpFrom'], jump_to=closure['jumpTo'],
            event=closure['event'], code=describe(function.__code__))
    monitoring = test_monitoring.TestBranchAndJumpEvents.test_callback_set_frame_lineno
    result['monitoring_branch'] = dict(
        source=inspect.getsource(monitoring),
        code=describe(nested_code(monitoring.__code__, 'func')))
    for name in ('test_setting_lineno_no_undefined',
                 'test_setting_lineno_one_undefined',
                 'test_setting_lineno_two_undefined'):
        function = getattr(test_peepholer.TestMarkingVariablesAsUnKnown, name)
        parsed = ast.parse(textwrap.dedent(inspect.getsource(function)))
        text = next(node.args[0].value for node in ast.walk(parsed)
                    if isinstance(node, ast.Call) and
                    isinstance(node.func, ast.Attribute) and
                    isinstance(node.func.value, ast.Name) and
                    node.func.value.id == 'textwrap' and node.func.attr == 'dedent' and
                    node.args and isinstance(node.args[0], ast.Constant))
        source = textwrap.dedent(text)
        module = compile(source, '<cancellation:' + name + '>', 'exec')
        result[name] = dict(source=source,
                            code=describe(nested_code(module, 'f')))
    print(json.dumps(dict(python=sys.version, compile_only=True,
                          cases=result), indent=2))


if __name__ == '__main__':
    main()
