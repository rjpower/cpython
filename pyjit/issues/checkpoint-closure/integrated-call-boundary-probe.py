"""Exercise actual object-vectorcall entries after the value-call ports."""

import operator

import _testcapi


def main():
    call = _testcapi.pyobject_vectorcall
    assert call(str.format, ("{}:{name}", 5, 6), ("name",)) == "5:6"
    assert call("{0.real}".format, (4.5,), None) == "4.5"
    assert call(dict.__getitem__, ({7: 4.5}, 7), None) == 4.5
    assert call(int.__new__, (int, 17), None) == 17
    assert call(operator.itemgetter(0), ((17,),), None) == 17
    assert call(operator.attrgetter("real"), (4.5,), None) == 4.5
    assert call(operator.methodcaller("bit_length"), (17,), None) == 5
    assert call(operator.methodcaller("to_bytes", length=1), (17,), None) == b"\x11"
    print("Integrated C object-vectorcall boundary probes passed")


if __name__ == "__main__":
    main()
