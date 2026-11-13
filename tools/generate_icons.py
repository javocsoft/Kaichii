#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

"""Writes the program icons from dist/branding/kaichii_icon.png (a square RGBA image, 1024 px):
dist/kaichii.ico (Windows, 16 to 256 px), dist/kaichii.png (Linux and the Qt resources, 256 px)
and dist/kaichii.icns (macOS, 128 to 1024 px). Needs Pillow."""

import argparse
import io
import pathlib
import struct

from PIL import Image, ImageFilter

ROOT = pathlib.Path(__file__).resolve().parent.parent
ICO_SIZES = (16, 24, 32, 48, 64, 128, 256)
ICNS_TYPES = ((b"ic07", 128), (b"ic08", 256), (b"ic09", 512), (b"ic10", 1024))


def render(source: Image.Image, size: int) -> bytes:
    image = source.resize((size, size), Image.LANCZOS)
    if size <= 32:
        # Downscaling softens the small sizes; this brings back the K's edges.
        image = image.filter(ImageFilter.UnsharpMask(radius=0.6, percent=60, threshold=0))
    out = io.BytesIO()
    image.save(out, format="PNG", optimize=True)
    return out.getvalue()


def write_ico(source: Image.Image, path: pathlib.Path) -> None:
    frames = [render(source, size) for size in ICO_SIZES]
    header = struct.pack("<HHH", 0, 1, len(frames))
    offset = len(header) + 16 * len(frames)
    entries = b""
    for size, frame in zip(ICO_SIZES, frames):
        entries += struct.pack("<BBBBHHII", size % 256, size % 256, 0, 0, 1, 32, len(frame), offset)
        offset += len(frame)
    path.write_bytes(header + entries + b"".join(frames))


def write_icns(source: Image.Image, path: pathlib.Path) -> None:
    chunks = b""
    for kind, size in ICNS_TYPES:
        frame = render(source, size)
        chunks += kind + struct.pack(">I", 8 + len(frame)) + frame
    path.write_bytes(b"icns" + struct.pack(">I", 8 + len(chunks)) + chunks)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=pathlib.Path,
                        default=ROOT / "dist" / "branding" / "kaichii_icon.png")
    args = parser.parse_args()
    source = Image.open(args.source).convert("RGBA")
    if source.width != source.height:
        raise SystemExit(f"{args.source}: the icon must be square, not {source.size}")
    dist = ROOT / "dist"
    write_ico(source, dist / "kaichii.ico")
    (dist / "kaichii.png").write_bytes(render(source, 256))
    write_icns(source, dist / "kaichii.icns")


if __name__ == "__main__":
    main()
