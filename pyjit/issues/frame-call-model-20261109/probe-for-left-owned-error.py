"""Coordinator-only probe for a FOR body result stranded by observer error."""
import dis
import gc
import json
import sys
import weakref


def case(raise_hook):
    refs, caught, callbacks = [], [], []
    class Resource:
        pass
    class Iterator:
        def __iter__(self):
            return self
        def __next__(self):
            value = Resource()
            refs.append(weakref.ref(value))
            return value
    class HookError(Exception):
        pass
    def consume():
        try:
            for value in Iterator():
                break
        except HookError:
            caught.append(True)
    for_sources = {instruction.offset for instruction in dis.get_instructions(consume)
                   if instruction.opname == 'FOR_ITER'}
    assert for_sources
    def branch(code, source, destination):
        if code is not consume.__code__ or source not in for_sources:
            return
        callbacks.append((source, destination))
        if raise_hook:
            raise HookError
    mon, tool = sys.monitoring, 4
    mon.use_tool_id(tool, 'for-result-owner-probe')
    try:
        mon.register_callback(tool, mon.events.BRANCH_LEFT, branch)
        mon.set_local_events(tool, consume.__code__, mon.events.BRANCH_LEFT)
        for _ in range(3):
            consume()
    finally:
        mon.set_local_events(tool, consume.__code__, 0)
        mon.register_callback(tool, mon.events.BRANCH_LEFT, None)
        mon.free_tool_id(tool)
    gc.collect()
    return {'raise_hook': raise_hook, 'created': len(refs),
            'alive_after_collect': sum(ref() is not None for ref in refs),
            'caught': len(caught), 'for_sources': sorted(for_sources), 'callbacks': callbacks}


print(json.dumps({'python': sys.version, 'results': [case(False), case(True)]}, indent=2))
