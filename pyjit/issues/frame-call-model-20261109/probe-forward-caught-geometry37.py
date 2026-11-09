"""Coordinator compile-only geometry of the exact forward-caught test sources."""
import argparse
import ast
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import types


METHODS = (
    'test_settrace_forward_caught_redirect_preserves_exception_owners',
    'test_settrace_forward_caught_refuses_missing_loop_owner',
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('test_source', type=Path)
    args = parser.parse_args()
    path = args.test_source.resolve()
    data = path.read_bytes()
    tree = ast.parse(data, filename=str(path))
    methods = {node.name: node for node in ast.walk(tree)
               if isinstance(node, ast.FunctionDef) and node.name in METHODS}
    assert set(methods) == set(METHODS)

    helper = Path(__file__).with_name('probe-debugger-context-inventory.py')
    spec = importlib.util.spec_from_file_location('_forward37_inventory', helper)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    rows = []
    for name in METHODS:
        assignments = [node for node in methods[name].body
                       if isinstance(node, ast.Assign)
                       and any(isinstance(target, ast.Name) and target.id == 'source'
                               for target in node.targets)]
        assert len(assignments) == 1
        source = ast.literal_eval(assignments[0].value)
        assert isinstance(source, str)
        lines = source.splitlines()
        markers = {kind: [i for i, line in enumerate(lines, 1)
                          if f'# {kind}' in line]
                   for kind in ('source', 'target')}
        assert all(len(found) == 1 for found in markers.values())
        source_line, target_line = markers['source'][0], markers['target'][0]
        root = compile(source, f'<forward-caught37:{name}>', 'exec')
        functions = [value for value in root.co_consts
                     if isinstance(value, types.CodeType) and value.co_name == 'f']
        assert len(functions) == 1
        described = module.describe(functions[0])
        words = described['words']
        assert len(words) < 512
        source_words = [word for word in words if word['position'][0] == source_line]
        target_words = [word for word in words if word['position'][0] == target_line]
        assert source_words and target_words
        loops = []
        for word in words:
            if word['operation'].startswith('FOR_ITER'):
                a, base, _, _ = word['fields']
                exit_pc = word['pc'] + 1 + word['oparg']
                loops.append(dict(header=word['pc'], body=word['pc'] + 1,
                                  exit=exit_pc, iterator=base, cursor=base + 1,
                                  item_destination=a,
                                  scratch=base >= described['nlocalsplus'],
                                  target_inside=any(word['pc'] < target['pc'] < exit_pc
                                                    for target in target_words)))
        rows.append(dict(method=name, extracted_source=source,
                         source_line=source_line, target_line=target_line,
                         source_words=source_words, target_words=target_words,
                         loop_windows=loops, code=described))
    print(json.dumps(dict(python=sys.version, compile_only=True,
                         fixture_bodies_executed=False, test_source=str(path),
                         test_source_sha256=hashlib.sha256(data).hexdigest(),
                         helper_sha256=hashlib.sha256(helper.read_bytes()).hexdigest(),
                         rows=rows), indent=2))


if __name__ == '__main__':
    main()
