"""Coordinator probe: target and callback own distinct handled exceptions."""
import json
import sys


def target():
    try:
        1 / 0
    except ZeroDivisionError:
        value = 1
        value = 2
    return value, sys.exception()


def describe(exc):
    return None if exc is None else {"type": type(exc).__name__, "id": id(exc)}


def main():
    first = target.__code__.co_firstlineno
    events = []
    applied = False
    def trace(frame, event, arg):
        nonlocal applied
        if (frame.f_code is target.__code__ and event == 'line' and
                frame.f_lineno == first + 5 and not applied):
            applied = True
            inherited = sys.exception()
            try:
                raise LookupError('callback-owned')
            except LookupError as callback:
                record = {'target_before': describe(inherited),
                          'callback_before': describe(sys.exception())}
                try:
                    frame.f_lineno = first + 6
                except ValueError as refusal:
                    record['refusal'] = str(refusal)
                record['callback_after'] = describe(sys.exception())
                record['callback_identity_preserved'] = sys.exception() is callback
            record['inherited_after_callback_except'] = describe(sys.exception())
            events.append(record)
        return trace
    old = sys.gettrace()
    sys.settrace(trace)
    try:
        value, handled = target()
    finally:
        sys.settrace(old)
    print(json.dumps({'applied': applied, 'value': value,
                      'target_after': describe(handled),
                      'events': events}, indent=2))


if __name__ == '__main__':
    main()
