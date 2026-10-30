import hashlib
import json
import marshal
import opcode
from pathlib import Path
import sys

binary = Path(sys.executable).resolve()
source = Path('/tmp/boilgate')
config = binary.parent / 'pyconfig.h'
print(json.dumps({'binary':str(binary), 'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),
    'config_sha256':hashlib.sha256(config.read_bytes()).hexdigest(),
    'opcode_module':opcode.__file__, 'sources':{str(path.relative_to(source)):
    hashlib.sha256(path.read_bytes()).hexdigest() for path in
    [source/'Objects/codeobject.c',source/'Include/internal/pycore_ir.h',source/'Include/opcode_ids.h']}}))
def f():
    return 'unique pristine marshal payload'
code=f.__code__
payload=marshal.dumps(code)
assert payload.count(code.co_code)==1
for name in ('INSTRUMENTED_LINE','BINARY_OP_ADD_INT','INTERPRETER_EXIT','EXIT_INIT_CHECK'):
    handler=opcode.opname.index(name)
    raw=handler.to_bytes(2,'little')+code.co_code[2:]
    for operation in ('replace','marshal'):
        try:
            result=code.replace(co_code=raw) if operation=='replace' else marshal.loads(payload.replace(code.co_code,raw,1))
        except ValueError as error:
            print(operation,name,'REJECTED',str(error))
        else:
            print(operation,name,'BUG: ACCEPTED; never executing malformed code',result.co_code[:2].hex())
