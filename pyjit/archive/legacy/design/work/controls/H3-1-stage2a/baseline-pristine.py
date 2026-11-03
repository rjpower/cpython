import opcode

code = (lambda: None).__code__
handler = opcode.opmap['INSTRUMENTED_LINE']
raw = handler.to_bytes(2, 'little') + code.co_code[2:]
try:
    constructed = code.replace(co_code=raw)
except ValueError:
    print('rejected runtime wrapper as pristine')
else:
    print('BUG: accepted pristine INSTRUMENTED_LINE', handler,
          'without monitoring state; not executing it')
    raise SystemExit(1)
