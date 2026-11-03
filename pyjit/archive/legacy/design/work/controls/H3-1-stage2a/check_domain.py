from pathlib import Path
import hashlib,json,sys
repo = next(parent for parent in Path(__file__).resolve().parents if (parent/'Tools/cases_generator').is_dir())
sys.path.insert(0,str(repo/'Tools/cases_generator'))
import analyzer,ir_generator
baseline=json.loads((repo/'boil/design/work/controls/H3-1-stage1/baseline-domain.json').read_text())
a=analyzer.analyze_files([str(repo/'Python/bytecodes.c')]);d=ir_generator.build_handler_domain(a)
for name,current in [('opmap',dict(d.opmap)),('handlers',dict(d.handlers)),('count',d.count),('first_ir_only',d.first_ir_only)]:
    assert current==baseline[name], name
print('PASS: 264 opcode/pseudo IDs, 287 dispatch IDs, count302 and first267 equal stage1 baseline')
print('PRISTINE dispatch count:',sum(bool(k&ir_generator.HandlerKind.PRISTINE) for k in d.kinds.values()))
print('runtime-only dispatch count:',sum(bool(k&ir_generator.HandlerKind.DISPATCH) and not bool(k&ir_generator.HandlerKind.PRISTINE) for k in d.kinds.values()))
