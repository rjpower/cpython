import gc
import io
import weakref

events = []
parent = None
armed = False
raw_ref = None
class Raw(io.RawIOBase):
    def readable(self):
        return True
    def __getattribute__(self, name):
        if name == 'seekable' and armed:
            detached = parent.detach()
            del detached
            def call():
                gc.collect()
                assert raw_ref() is not None
                events.append('raw-called')
                return 1234567
            return call
        return super().__getattribute__(name)
    def __del__(self):
        events.append('raw-del')
raw = Raw()
raw_ref = weakref.ref(raw)
parent = io.BufferedReader(raw)
del raw
armed = True
assert parent.seekable() == 1234567
assert events == ['raw-called', 'raw-del'], events

events.clear()
decoder_parent = None
decoder_ref = None
class Decoder:
    def __getattribute__(self, name):
        if name == 'reset':
            decoder_parent.__init__(None, True)
            def call():
                gc.collect()
                assert decoder_ref() is not None
                events.append('decoder-called')
                return 1234567
            return call
        return object.__getattribute__(self, name)
    def __del__(self):
        events.append('decoder-del')
decoder = Decoder()
decoder_ref = weakref.ref(decoder)
decoder_parent = io.IncrementalNewlineDecoder(decoder, True)
del decoder
assert decoder_parent.reset() == 1234567
assert events == ['decoder-called', 'decoder-del'], events

class Broken(io.RawIOBase):
    def readable(self): return True
    def seekable(self): raise LookupError('seekable failure')
try:
    io.BufferedReader(Broken()).seekable()
except LookupError as exc:
    assert str(exc) == 'seekable failure'
else:
    raise AssertionError('expected method error')
print('I/O no-argument receiver lifetime and error probes passed')
class Direct(io.RawIOBase):
    def readall(self): return 7654321
    def close(self): return 3456789
r = Direct()
assert r.read() == 7654321
assert r.__exit__(None, None, None) == 3456789
print('I/O self-receiver value return probes passed')
