#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Side-by-side comparison sheet of screen PNGs, scaled up for a normal monitor (P1-04 mockups).

    python3 tools/mockup_sheet.py OUT.png SCALE IN1.png IN2.png ...

Reads the 240x240 8-bit RGB PNGs that host/png.c writes, puts them in a row with a 16 px gap,
each pixel drawn SCALE x SCALE. Python 3.9+, standard library only.
"""
import struct
import sys
import zlib
from pathlib import Path


def read(path):
    b = Path(path).read_bytes()
    pos, idat, w = 8, b"", 0
    while pos < len(b):
        n, = struct.unpack(">I", b[pos:pos + 4])
        kind, data = b[pos + 4:pos + 8], b[pos + 8:pos + 8 + n]
        if kind == b"IHDR":
            w, h = struct.unpack(">II", data[:8])
        elif kind == b"IDAT":
            idat += data
        pos += 12 + n
    raw = zlib.decompress(idat)
    rows = []
    for y in range(h):
        r = raw[y * (1 + 3 * w):(y + 1) * (1 + 3 * w)]
        assert r[0] == 0, "filter 0 only"
        rows.append(r[1:])
    return w, h, rows


def chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))


def main(out, scale, ins):
    imgs = [read(p) for p in ins]
    gap, bg = 16, bytes((0x30, 0x30, 0x30))
    h = max(i[1] for i in imgs) * scale
    w = sum(i[0] * scale for i in imgs) + gap * (len(imgs) + 1)
    raw = bytearray()
    for y in range(h + 2 * gap):
        raw.append(0)
        line = bytearray(bg * gap)
        for iw, ih, rows in imgs:
            yy = (y - gap) // scale
            if 0 <= y - gap < ih * scale:
                src = rows[yy]
                for x in range(iw):
                    line += src[3 * x:3 * x + 3] * scale
            else:
                line += bg * (iw * scale)
            line += bg * gap
        raw += line
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h + 2 * gap, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b"")
    Path(out).write_bytes(png)


if __name__ == "__main__":
    if len(sys.argv) < 4:
        sys.exit(__doc__)
    main(sys.argv[1], int(sys.argv[2]), sys.argv[3:])
