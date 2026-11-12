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
        flags = int(gdb.parse_and_eval('code->co_flags')) & (32 | 128 | 512)
        line = int(gdb.parse_and_eval('code->co_firstlineno'))
        names = []
        frame = gdb.newest_frame().older()
        for _ in range(8):
            if frame is None:
                break
            names.append(frame.name() or '<unknown>')
            frame = frame.older()
        path = 'other'
        if any('_PyFunction_ValueVectorcall' in name for name in names):
            path = 'native-value-vector'
        elif any('_PyEval_Vector' in name for name in names):
            path = 'native-object-vector'
        elif any('call_bind_known_python' in name or 'CALL_PY_KNOWN' in name for name in names):
            path = 'known-positional'
        elif any('CALL_PY_EXACT_ARGS' in name for name in names):
            path = 'ir-exact'
        counts[(path, flags, line)] += 1
        return False
probe = Construction('_Py_MakeCoroFromCode')
probe.enabled = False
end
run
python
probe.enabled = True
end
continue
python
print('CONSTRUCTOR_HITS ' + json.dumps([{'path': key[0], 'flags': key[1], 'firstlineno': key[2], 'count': count} for key, count in sorted(counts.items())]))
end
