"""Builders for deliberate serialized IR recovery/position test images."""

import struct


def semantic_table(exceptions=b"", records=()):
    records = tuple(records)
    if not exceptions and not records:
        return b""
    return (b"IRSC" + struct.pack("<III", 1, len(exceptions), len(records)) +
            exceptions + b"".join(struct.pack("<6i", *row) for row in records))


def semantic_parts(data):
    if not data:
        return b"", []
    magic, version, size, count = struct.unpack_from("<4sIII", data)
    assert (magic, version) == (b"IRSC", 1)
    assert len(data) == 16 + size + 24 * count
    return data[16:16 + size], list(struct.iter_unpack("<6i", data[16 + size:]))


def position_table(locations=b"", records=()):
    records = tuple(records)
    return (b"IRSM" + struct.pack("<III", 2, len(locations), len(records)) +
            locations + b"".join(struct.pack("<4i", *row) for row in records))


def position_parts(data):
    magic, version, size, count = struct.unpack_from("<4sIII", data)
    assert (magic, version) == (b"IRSM", 2)
    assert len(data) == 16 + size + 16 * count
    return data[16:16 + size], list(struct.iter_unpack("<4i", data[16 + size:]))


def observation_records(code):
    import _opcode
    _, positions = position_parts(code.co_linetable)
    recovery = _opcode.get_recovery_entries(code)
    assert not positions or len(positions) == len(recovery)
    return [(*row[:3], *(positions[i] if positions else (-1,) * 4), *row[3:])
            for i, row in enumerate(recovery)]


def observation_tables(locations, records, exceptions=b""):
    """Split deliberate ten-field observer fixtures into the two containers."""
    records = tuple(records)
    return {
        "co_exceptiontable": semantic_table(
            exceptions, [(*row[:3], *row[7:]) for row in records]),
        "co_linetable": position_table(locations, [row[3:7] for row in records]),
    }
