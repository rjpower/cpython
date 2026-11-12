set pagination off
set confirm off
set auto-load python-scripts off
handle SIGUSR1 noprint stop nopass
python
import collections
import gdb
import json
counts = collections.Counter()
class Construction(gdb.Breakpoint):
    def stop(self):
        # This allocation call is reached only after constructor admission.
        # The unchanged diagnostic workloads do not inject allocation failure.
        frame = gdb.newest_frame().older()
        if frame is not None and frame.name() == '_PyGen_TryMakeFromFunction':
            flags = int(gdb.parse_and_eval('code->co_flags')) & (32 | 128 | 512)
            line = int(gdb.parse_and_eval('code->co_firstlineno'))
            counts[(flags, line)] += 1
        return False
probe = Construction('make_gen')
probe.enabled = False
end
run
python
probe.enabled = True
end
continue
python
print('NATIVE_CONSTRUCTOR_HITS ' + json.dumps([{'flags': key[0], 'firstlineno': key[1], 'count': count} for key, count in sorted(counts.items())]))
end
