"""Check frozen IDs and agreement of all emitted names/private metadata."""

import json
from pathlib import Path
import re
import sys

repo = next(p for p in Path(__file__).resolve().parents
            if (p / "Python/bytecodes.c").is_file())
sys.path.insert(0, str(repo / "Tools/cases_generator"))
import analyzer
import ir_generator
import _opcode_metadata as python

baseline = json.loads((repo / "boil/design/work/controls/H3-1-stage1/baseline-domain.json")
                      .read_text())
analysis = analyzer.analyze_files([str(repo / "Python/bytecodes.c")])
domain = ir_generator.build_handler_domain(analysis)
for name, value in (("opmap", dict(domain.opmap)), ("handlers", dict(domain.handlers)),
                    ("count", domain.count), ("first_ir_only", domain.first_ir_only)):
    assert value == baseline[name], name
header = (repo / "Include/internal/pycore_opcode_metadata.h").read_text()
table = header.split("const char *_PyOpcode_OpName[IR_NUM_HANDLERS] = {", 1)[1].split("};", 1)[0]
names = {int(index): name for index, name in re.findall(r'\[(\d+)\] = "([^"\n]+)",', table)}
ids = {**domain.opmap, **domain.handlers}
expected = {handler: name for name, handler in ids.items()
            if domain.kinds[name] & ir_generator.HandlerKind.METADATA}
assert names == expected, (names, expected)
for name, handler in ids.items():
    assert python._handler_bases[handler] == ids[domain.bases[name]], name
    assert python._handler_flags[handler] == domain.kinds[name], name
    assert python._handler_operations[handler] == domain.operations.get(name), name
for source in ("Python/ceval.h", "Modules/_testinternalcapi.c", "Include/internal/pycore_ir_metadata.h"):
    assert "_PyIR_OnlyNames" not in (repo / source).read_text(), source
assert "_IR_FIRST_ONLY_HANDLER" not in (repo / "Lib/dis.py").read_text()
print(json.dumps({"count": domain.count, "opcodes_pseudos": len(domain.opmap),
                  "dispatch": len(domain.handlers), "metadata_names": len(names),
                  "first_ir_only": domain.first_ir_only,
                  "status": "frozen IDs and all C/Python/domain metadata agree"}))
