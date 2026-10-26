import importlib
import sys


def run_tests(testmod, verbose=True):
    for name in dir(testmod):
        if not name.startswith('test'):
            continue
        func = getattr(testmod, name)
        print(f"{name}()")
        func()

    print("add()")
    if testmod.add(11, 23) != 34:
        raise AssertionError("add() failed badly")

    print(flush=True)


def main():
    if len(sys.argv) < 2:
        print("usage: python runtests.py TEST_MODULE_NAME")
        sys.exit(1)
    module_name = sys.argv[1]

    testmod = importlib.import_module(module_name)

    newline = False
    for name in (
        '__STDC_VERSION__',
        '__STRICT_ANSI__',
        '__cplusplus',
        '_MSVC_LANG',
    ):
        try:
            value = getattr(testmod, name)
        except AttributeError:
            pass
        else:
            print(f'{name}: {value}')
            newline = True
    if newline:
        print()

    run_tests(testmod)


if __name__ == "__main__":
    main()
