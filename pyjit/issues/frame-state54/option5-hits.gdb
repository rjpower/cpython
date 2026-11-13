# Untimed native census only. Launch with -iex 'set auto-load python-scripts off'.
# Use option5-hit-driver.py: its first SIGUSR1 ends warmup, its second ends run(1).
set pagination off
set confirm off
set auto-load python-scripts off
handle SIGUSR1 noprint stop nopass
python
import collections
import gdb
import hashlib
import json
import subprocess

expected_head = '4fbd16a14621f2b5327a0bfde6c2198190e71428'
expected_sha256 = 'd7d0f5ae2bb7bf1aeec248de14700a165a66005b4e619ab5d0cd3c30a6723c00'
source = '/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/frame54-clears'
head = subprocess.check_output(
    ['git', '-C', source, 'rev-parse', 'HEAD'], text=True).strip()
assert head == expected_head, (head, expected_head)
assert not subprocess.check_output(
    ['git', '-C', source, 'status', '--porcelain'], text=True).strip()
elf = gdb.current_progspace().filename
digest = hashlib.sha256()
with open(elf, 'rb') as stream:
    for block in iter(lambda: stream.read(1024 * 1024), b''):
        digest.update(block)
assert digest.hexdigest() == expected_sha256, (elf, digest.hexdigest())

counts = collections.Counter(
    generic_entries=0, refined_entries=0, successful_overwrites=0)
state = {'signal': None, 'exit_code': None}

class Hit(gdb.Breakpoint):
    def __init__(self, location, key):
        super().__init__(location, internal=True)
        self.key = key
        self.enabled = False

    def stop(self):
        counts[self.key] += 1
        return False

generic = Hit('*_TAIL_CALL_BINARY_OP_ADD_INT', 'generic_entries')
refined = Hit('*_TAIL_CALL_BINARY_OP_ADD_INT_OVERWRITE_TEMP', 'refined_entries')
success = Hit('*_TAIL_CALL_BINARY_OP_ADD_INT_OVERWRITE_TEMP+0xd7',
              'successful_overwrites')
probes = (generic, refined, success)
assert all(len(probe.locations) == 1 for probe in probes)
assert success.locations[0].address - refined.locations[0].address == 0xd7

def stopped(event):
    state['signal'] = event.stop_signal if isinstance(event, gdb.SignalEvent) else None

def exited(event):
    state['exit_code'] = event.exit_code

gdb.events.stop.connect(stopped)
gdb.events.exited.connect(exited)
identity = {'kind': 'diagnostic-not-timing', 'source_head': head,
            'elf': elf, 'elf_sha256': digest.hexdigest(),
            'success_offset': '0xd7', 'loops': 1, 'warmup_loops': 1}
print('OPTION5_HITS_IDENTITY ' + json.dumps(identity, sort_keys=True))
end
run
python
assert state['signal'] == 'SIGUSR1' and state['exit_code'] is None, state
assert bytes(gdb.selected_inferior().read_memory(
    success.locations[0].address, 3)) == bytes.fromhex('49 89 3f')
for probe in probes:
    probe.enabled = True
state['signal'] = None
end
continue
python
assert state['signal'] == 'SIGUSR1' and state['exit_code'] is None, state
for probe in probes:
    probe.enabled = False
state['signal'] = None
end
continue
python
assert state['exit_code'] == 0 and state['signal'] is None, state
assert counts['successful_overwrites'] <= counts['refined_entries'], counts
print('OPTION5_HITS ' + json.dumps(
    dict(identity, counts=dict(counts)), sort_keys=True))
end
