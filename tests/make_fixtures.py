#!/usr/bin/env python3
"""Gera os fixtures de teste: JPEGs minimos com e sem EXIF DateTime."""
import os
import struct

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "data")


def tiff_ifd0(datetime_str: str, be: bool) -> bytes:
    """TIFF com um IFD0 contendo apenas a tag 0x0132 (DateTime, ASCII, 20 bytes)."""
    e = ">" if be else "<"
    ascii_bytes = datetime_str.encode() + b"\0"           # 19 + 1 = 20 bytes
    # layout: header(8) | IFD(2 + 12 + 4) | valor(20)
    ifd_off = 8
    val_off = ifd_off + 2 + 12 + 4
    hdr = (b"MM" if be else b"II") + struct.pack(e + "HI", 42, ifd_off)
    entry = struct.pack(e + "HHI", 0x0132, 2, len(ascii_bytes)) + struct.pack(e + "I", val_off)
    ifd = struct.pack(e + "H", 1) + entry + struct.pack(e + "I", 0)
    return hdr + ifd + ascii_bytes


def jpeg_with_exif(datetime_str: str, be: bool) -> bytes:
    tiff = tiff_ifd0(datetime_str, be)
    body = b"Exif\0\0" + tiff
    app1 = b"\xFF\xE1" + struct.pack(">H", len(body) + 2) + body
    # APP0/JFIF + EOI so para parecer um JPEG de verdade
    app0_body = b"JFIF\0\x01\x01\x00\x00\x01\x00\x01\x00\x00"
    app0 = b"\xFF\xE0" + struct.pack(">H", len(app0_body) + 2) + app0_body
    return b"\xFF\xD8" + app1 + app0 + b"\xFF\xD9"


def jpeg_without_exif() -> bytes:
    app0_body = b"JFIF\0\x01\x01\x00\x00\x01\x00\x01\x00\x00"
    app0 = b"\xFF\xE0" + struct.pack(">H", len(app0_body) + 2) + app0_body
    return b"\xFF\xD8" + app0 + b"\xFF\xD9"


def main() -> None:
    os.makedirs(OUT, exist_ok=True)
    cases = {
        "exif_le.jpg": jpeg_with_exif("2026:11:13 22:15:23", be=False),
        "exif_be.jpg": jpeg_with_exif("2019:07:04 09:05:06", be=True),
        "noexif.jpg": jpeg_without_exif(),
        "notjpeg.bin": b"\x00\x01\x02\x03" * 8,
    }
    for name, data in cases.items():
        with open(os.path.join(OUT, name), "wb") as f:
            f.write(data)
        print(f"{name}: {len(data)} bytes")


if __name__ == "__main__":
    main()
