"""Native constructor failure probe, run in a fresh candidate process per kind."""

import _testcapi
import gc
import json
import sys
import warnings
import weakref


def invoke(function, value):
    return function(value)


def main():
    namespace = {}
    if sys.argv[1] == 'generator':
        exec('def make(value):\n yield value\n', namespace)
    else:
        assert sys.argv[1] == 'coroutine'
        exec('async def make(value):\n return value\n', namespace)
    make = namespace['make']
    class Payload:
        pass
    value = Payload()
    reference = weakref.ref(value)
    for _ in range(64):
        created = invoke(make, value)
        created.close()
    failed = False
    with warnings.catch_warnings(record=True) as caught:
        warnings.simplefilter('always')
        _testcapi.set_nomemory(0, 1)
        try:
            unexpected = invoke(make, value)
        except MemoryError:
            failed = True
        finally:
            _testcapi.remove_mem_hooks()
        if not failed:
            unexpected.close()
            raise AssertionError('the next allocation did not fail')
        gc.collect()
    assert reference() is value
    assert not caught, [str(warning.message) for warning in caught]
    del value
    gc.collect()
    assert reference() is None, 'a failed call retained an input owner'
    print(json.dumps({'kind': sys.argv[1], 'memory_error': failed,
                      'warnings': len(caught), 'input_released': True,
                      'git': list(sys._git)}))


if __name__ == '__main__':
    main()
