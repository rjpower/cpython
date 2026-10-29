import difflib
import hashlib
import json
import os
from pathlib import Path
import subprocess

repo=Path.cwd()
source_path=Path('Tools/cases_generator/ir_generator.py')
original=source_path.read_text()
controls=(
 ('duplicate','test_duplicate_id_rejected',
  '        if len(set(ids.values())) != len(ids):\n            raise ValueError("duplicate handler ID")\n',''),
 ('capacity','test_capacity_without_large_tables',
  '    if not 0 < count <= 1 << 16:', '    if not 0 < count:'),
 ('consumer','test_consumers_reject_another_analysis',
  '        if analysis is not self.analysis:\n            raise ValueError("handler domain belongs to a different analysis")', '        pass'),
 ('base','test_base_metadata_rejected',
  '        for name, base in self.bases.items():\n            if base not in ids or self.bases[base] != base:\n                raise ValueError(f"invalid handler base for {name}: {base}")\n', ''),
 ('kind','test_base_does_not_classify_specialization',
  '        kinds[name] |= HandlerKind.BINARY_OP\n',
  '        kinds[name] |= HandlerKind.BINARY_OP | HandlerKind.SPECIALIZED\n'),
 ('pin','test_absent_instrumented_pin',
  '    min_instrumented = 256 - len(set(instrumented) | pinned_instrumented.keys())',
  '    min_instrumented = 256 - len(instrumented)'),
 ('name','test_derived_name_collision_rejected',
  '        if name in opmap or name in handlers:\n            raise ValueError(f"duplicate handler name: {name}")\n', ''),
)
results=[]
out=Path('build/H3-1/controls');out.mkdir(exist_ok=True)
for label,test,old,new in controls:
 assert original.count(old)==1,(label,original.count(old))
 variant=original.replace(old,new)
 if label == 'name':
  # Either the builder or the product rejects a name assigned two IDs.
  # Remove both checks to demonstrate the invariant, not just a diagnostic.
  variant=variant.replace('            if name in ids and ids[name] != handler:\n                raise ValueError(f"conflicting handler ID for {name}")\n','')
 (out/(label+'.patch')).write_text(''.join(difflib.unified_diff(original.splitlines(True),variant.splitlines(True),fromfile='a/'+str(source_path),tofile='b/'+str(source_path))))
 # Each variant exists only while its own locked test runs. No C build or
 # unrelated suite is run with changed generator source.
 try:
  source_path.write_text(variant)
  command=['uv','run','--no-project','--','boil/tools/gate_run.sh',str(repo/'build/default/.gate.lock'),str(repo/'build/default/python'),str(repo/'build/default/python'),'-m','test','-v','-m','*'+test,'test_generated_cases']
  env=dict(os.environ, UV_CACHE_DIR='/home/power/.cache/uv',PYTHONPYCACHEPREFIX=str(repo/'build/H3-1/control-pyc'/label))
  with (out/(label+'.log')).open('w') as stream:
   completed=subprocess.run(command,env=env,stdout=stream,stderr=subprocess.STDOUT,check=False)
 finally:
  source_path.write_text(original)
 log=(out/(label+'.log')).read_text()
 assert completed.returncode!=0 and ('FAIL' in log or 'ERROR' in log),(label,completed.returncode,log)
 results.append({'control':label,'test':test,'exit':completed.returncode,'source_sha256':hashlib.sha256(variant.encode()).hexdigest(),'patch_sha256':hashlib.sha256((out/(label+'.patch')).read_bytes()).hexdigest()})
 print(label,'detected',completed.returncode,flush=True)
(out/'manifest.json').write_text(json.dumps({'base':'f46276debf3','restored_source_sha256':hashlib.sha256(original.encode()).hexdigest(),'controls':results},indent=2)+'\n')
assert source_path.read_text()==original
