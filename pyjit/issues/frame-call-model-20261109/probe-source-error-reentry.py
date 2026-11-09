"""Run with the candidate interpreter to exercise nested source callback errors."""

import json
import sys


events = []


class OuterError(Exception):
    pass


class InnerError(Exception):
    pass


SOURCE = '''
def nested():
    try:
        pass  # nested source event
    except InnerError:
        events.append("inner caught")

class Returned:
    def __del__(self):
        events.append("retire")
        nested()
        events.append("retired")

def values():
    yield 1
    return Returned()

def outer():
    for item in values():
        pass
    else:
        try:
            pass  # outer source event
        except OuterError:
            events.append("outer caught")
'''

exec(compile(SOURCE, '<source-error-reentry>', 'exec'), globals())
OUTER_LINE = next(i for i, line in enumerate(SOURCE.splitlines(), 1)
                  if '# outer source event' in line)
INNER_LINE = next(i for i, line in enumerate(SOURCE.splitlines(), 1)
                  if '# nested source event' in line)


def run(nested_failure, warmup):
    for _ in range(warmup):
        outer()
    events.clear()
    tool = 2
    sys.monitoring.use_tool_id(tool, 'source-error-reentry')

    def callback(code, line):
        if code is outer.__code__ and line == OUTER_LINE:
            events.append('outer event')
            raise OuterError('outer callback')
        if nested_failure and code is nested.__code__ and line == INNER_LINE:
            events.append('inner event')
            raise InnerError('inner callback')

    sys.monitoring.register_callback(tool, sys.monitoring.events.LINE, callback)
    for function in (outer, nested):
        sys.monitoring.set_local_events(tool, function.__code__,
                                        sys.monitoring.events.LINE)
    escaped = []
    try:
        outer()
    except OuterError as error:
        traceback = error.__traceback__
        while traceback is not None:
            escaped.append((traceback.tb_frame.f_code.co_name, traceback.tb_lineno))
            traceback = traceback.tb_next
    finally:
        for function in (outer, nested):
            sys.monitoring.set_local_events(tool, function.__code__, 0)
        sys.monitoring.register_callback(tool, sys.monitoring.events.LINE, None)
        sys.monitoring.free_tool_id(tool)
    return dict(nested_failure=nested_failure, warmup=warmup,
                events=events.copy(), escaped=escaped)


def main():
    results = [run(nested_failure, warmup)
               for warmup in (0, 20) for nested_failure in (False, True)]
    print(json.dumps(results))
    if '--require-fixed' in sys.argv:
        for result in results:
            assert not result['escaped'], result
            assert 'outer caught' in result['events'], result
            if result['nested_failure']:
                assert 'inner caught' in result['events'], result


if __name__ == '__main__':
    main()
