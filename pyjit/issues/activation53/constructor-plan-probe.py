"""Small positional-plan hit probe; diagnostic only, not a performance kernel."""


def planned(value, optional=17, *args, **kwargs):
    yield value, optional, args, kwargs


def run(loops):
    for _ in range(64 * loops):
        generator = planned(11)
        assert next(generator) == (11, 17, (), {})
        generator.close()
