import json
import os
import runpy
import sys

import _testinternalcapi

actual = _testinternalcapi.gc_verify_mode()
assert actual == (2, 0), (actual, sys.flags.ignore_environment, sys.orig_argv[:3])
assert sys.argv[1] == '-m', sys.argv[:3]
record = {'pid': os.getpid(), 'mode': actual,
          'ignore_environment': sys.flags.ignore_environment,
          'module': sys.argv[2]}
with open(os.environ['BOIL_MODE_EVIDENCE'], 'a') as evidence:
    evidence.write(json.dumps(record) + '\n')
module = sys.argv[2]
sys.argv = sys.argv[2:]
runpy.run_module(module, run_name='__main__', alter_sys=True)
