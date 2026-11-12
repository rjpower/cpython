"""Native provider/thunk constructor census; diagnostic, not a timed kernel."""

import _testcapi


def generator(value):
    yield value


async def coroutine(value):
    return value


def run(loops):
    for _ in range(64 * loops):
        for factory in (generator, coroutine):
            for value in (next(map(factory, (17,))),
                          _testcapi.pyobject_vectorcall(factory, (17,), None)):
                if factory is generator:
                    assert next(value) == 17
                    value.close()
                else:
                    try:
                        value.send(None)
                    except StopIteration as result:
                        assert result.value == 17
                    else:
                        raise AssertionError('coroutine did not finish')
