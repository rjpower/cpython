"""Exercise the GDB location reader without requiring an inferior process."""

import ast
from pathlib import Path
import struct
import unittest
from test import test_tools


@unittest.skipIf(test_tools.basepath is None, 'needs the source tree')
class LocationParserTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = Path(test_tools.basepath) / 'Tools' / 'gdb' / 'libpython.py'
        tree = ast.parse(source.read_text())
        names = {'read', 'read_varint', 'read_signed_varint', 'parse_location_table'}
        functions = [node for node in tree.body
                     if isinstance(node, ast.FunctionDef) and node.name in names]
        namespace = {}
        exec(compile(ast.Module(body=functions, type_ignores=[]), str(source), 'exec'), namespace)
        cls.parse = staticmethod(namespace['parse_location_table'])

    def test_actual_proxy_character_input(self):
        payload = b'\x80\x00'
        wrapped = b'IRSM' + struct.pack('<III', 2, len(payload), 0) + payload
        expected = [(0, 1, 10)]
        self.assertEqual(list(self.parse(10, payload.decode('latin-1'))), expected)
        self.assertEqual(list(self.parse(10, wrapped.decode('latin-1'))), expected)

    def test_truncated_and_inconsistent_envelopes(self):
        for raw in (b'IRSM', b'IRSM' + struct.pack('<III', 1, 0, 0),
                    b'IRSM' + struct.pack('<III', 2, 1, 0),
                    b'IRSM' + struct.pack('<III', 2, 0, 1)):
            with self.subTest(raw=raw):
                with self.assertRaisesRegex(ValueError, 'source map'):
                    list(self.parse(10, raw.decode('latin-1')))


if __name__ == '__main__':
    unittest.main()
