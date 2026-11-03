"""Point a regrtest driver or worker at a task-owned generator snapshot."""

import hashlib
import importlib
from pathlib import Path
import runpy
import sys

# Verbose regrtest asks its --python command for host platform information.
# This probe does not import tests or interpret handler IDs.
if sys.argv[3:] == ["-m", "platform"]:
    sys.argv = ["platform"]
    runpy.run_module("platform", run_name="__main__")
    raise SystemExit(0)

from test import test_tools


copied_root = Path(sys.argv[1]).resolve()
expected = sys.argv[2]
test_tools.toolsdir = str(copied_root / "Tools")
is_worker = sys.argv[-3:-1] == ["-m", "test.libregrtest.worker"]
with test_tools.imports_under_tool("cases_generator"):
    for name in ("analyzer", "ir_generator", "opcode_id_generator",
                 "opcode_metadata_generator", "py_metadata_generator",
                 "uop_id_generator", "ir_metadata_generator"):
        module = importlib.import_module(name)
        assert Path(module.__file__).resolve().is_relative_to(copied_root)
    import ir_generator
    actual = hashlib.sha256(Path(ir_generator.__file__).read_bytes()).hexdigest()
    assert actual == expected, (actual, expected)
role = "worker" if is_worker else "driver"
print("isolated", role, "generator verified", ir_generator.__file__, actual,
      flush=True)

if is_worker:
    from test.libregrtest.worker import main
    sys.argv = ["test.libregrtest.worker", sys.argv[-1]]
    main()
else:
    assert sys.argv[3:5] == ["-m", "test"], sys.argv
    sys.argv = ["test"] + sys.argv[5:]
    runpy.run_module("test", run_name="__main__")
