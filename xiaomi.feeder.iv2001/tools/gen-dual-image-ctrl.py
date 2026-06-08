#!/usr/bin/env python3
"""Emit dual_image_ctrl_bank_a.bin for manual recovery @ 0x0800F000."""

import struct
from pathlib import Path

BOOT_CTRL_MAGIC = 0x4455414C  # "DUAL"
BOOT_FLAG_A = 0xABCDDCBA

REPO = Path(__file__).resolve().parents[1]
OUT = REPO / "firmware" / "flash" / "dual_image_ctrl_bank_a.bin"

OUT.write_bytes(struct.pack("<II", BOOT_CTRL_MAGIC, BOOT_FLAG_A) + (b"\xff" * 64))
print(f"wrote {OUT} ({OUT.stat().st_size} bytes)")
