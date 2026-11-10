"""Check that cached class lookup acquires its result before receiver cleanup."""

import gc
import sys
import weakref


def check(slotted):
    events = []
    armed = False

    class Value:
        def __del__(self):
            events.append("value retired")

    if slotted:
        class Receiver:
            __slots__ = ()

            def __del__(self):
                if armed:
                    del type(self).value
    else:
        class Receiver:
            def __del__(self):
                if armed:
                    del type(self).value

    def read():
        return Receiver().value

    Receiver.value = [Value()]
    for _ in range(100):
        read()
    if "--show-ir" in sys.argv:
        import _testinternalcapi
        print(slotted, _testinternalcapi.get_ir(read.__code__, True), flush=True)
    reference = weakref.ref(Receiver.value[0])
    armed = True
    result = read()
    assert reference() is result[0]
    assert events == []
    del result
    gc.collect()
    assert events == ["value retired"]


check(False)
check(True)
print("lookup result lifetime controls passed")
