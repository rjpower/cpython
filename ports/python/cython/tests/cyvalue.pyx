# cython: language_level=3
"""Exercises the code Cython generates against the PyValue C API."""

cimport cython


def nullary():
    return "nullary"


def unary(a):
    return a


@cython.always_allow_keywords(False)
def unary_o(a):
    # METH_O
    return (a, type(a).__name__)


def nary(a, b, c):
    return a + b + c


def kw(a, b=2, *, c=3, **rest):
    return (a, b, c, sorted(rest.items()))


def star(*args, **kwargs):
    return (args, sorted(kwargs.items()))


def typed(int i, double d, long long big):
    return i * 2, d / 2, big + 1


cdef class Counter:
    """An iterator over range(n) with __getitem__ and __richcmp__."""
    cdef public int n
    cdef int i

    def __init__(self, n):
        self.n = n
        self.i = 0

    def __iter__(self):
        return self

    def __next__(self):
        if self.i >= self.n:
            raise StopIteration
        self.i += 1
        if self.i % 2:
            return self.i - 1
        return (self.i - 1) * 0.5

    def __getitem__(self, key):
        return ("item", key)

    def __richcmp__(self, other, int op):
        if op == 2:
            return isinstance(other, Counter) and (<Counter>other).n == self.n
        if op == 0:
            return self.n < (<Counter>other).n
        return NotImplemented

    def __class_getitem__(cls, item):
        return ("class_getitem", cls.__name__, item)

    cpdef int twice(self, int x):
        return 2 * x + self.n

    def method(self, a, b=1.5):
        return a * b

    @cython.always_allow_keywords(False)
    def method_o(self, a):
        return a

    @classmethod
    def make(cls, n):
        return cls(n)

    @staticmethod
    def static(a, b):
        return a - b


class Sub(Counter):
    def twice(self, x):
        return -x


def gen(n):
    for i in range(n):
        yield i
        yield i + 0.5
    return "done"


async def coro(x):
    return x * 2


async def awaiter(x):
    return await coro(x) + 1


def closure(k):
    def inner(x):
        return x * k
    return inner


def use_lambda(seq):
    return sorted(seq, key=lambda v: -v)


def catch(f, *args):
    try:
        return f(*args)
    except ZeroDivisionError as e:
        return "zde"
    except (KeyError, IndexError):
        return "lookup"
    finally:
        pass


def comps(n):
    ints = [i * i for i in range(n)]
    floats = [i / 4 for i in range(n)]
    d = {i: f for i, f in zip(ints, floats)}
    s = {f for f in floats}
    g = sum(x for x in ints)
    return ints, floats, d, s, g


def iterate(obj):
    out = []
    for x in obj:
        out.append(x)
    return out


def call(f, *args, **kwargs):
    return f(*args, **kwargs)


def builtins_use(seq):
    return len(seq), max(seq), min(seq), list(reversed(seq)), tuple(seq), next(iter(seq))


ctypedef fused number:
    int
    double


def fused_add(number a, number b):
    return a + b


cpdef module_cpdef(x):
    return x


cdef class Special:
    cdef public object v

    def __init__(self, v):
        self.v = v

    def __getattr__(self, name):
        return ("getattr", name)

    def __add__(self, other):
        return ("add", other)

    def __call__(self, *args, **kwargs):
        return (args, sorted(kwargs.items()))

    @staticmethod
    def one(x):
        return x

    @property
    def prop(self):
        return self.v


def delegate(n):
    yield from gen(n)


async def agen(n):
    for i in range(n):
        yield i * 0.5


async def consume(n):
    out = []
    async for x in agen(n):
        out.append(x)
    return out


def unpack(seq):
    a, b, *rest = seq
    x, y = iter(seq[:2])
    return a, b, rest, x, y


class PyIter:
    def __init__(self, n):
        self.n = n

    def __iter__(self):
        return self

    def __next__(self):
        if self.n <= 0:
            raise StopIteration
        self.n -= 1
        return self.n + 0.5


def pops(list L, Py_ssize_t i):
    a = L.pop(i)
    b = L.pop()
    return a, b, L, L[1:], tuple(L)[1:]


def match_seq(x):
    match x:
        case (a, *rest):
            return a, rest
        case _:
            return None
