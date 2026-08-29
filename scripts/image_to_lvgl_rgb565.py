#!/usr/bin/env python3
"""Convert a RGB image to a LVGL 9 RGB565 C asset.

Usage:
  python scripts/image_to_lvgl_rgb565.py input.png output.c symbol
"""
from pathlib import Path
import sys
from PIL import Image

if len(sys.argv) != 4:
    raise SystemExit("usage: image_to_lvgl_rgb565.py INPUT OUTPUT SYMBOL")
input_path, output_path, symbol = sys.argv[1:]
img = Image.open(input_path).convert("RGB")
w, h = img.size
raw = bytearray()
for r, g, b in img.getdata():
    v = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
    raw.append(v & 0xff)
    raw.append((v >> 8) & 0xff)
lines=[]
for i in range(0, len(raw), 16):
    lines.append("    " + ",".join(f"0x{x:02x}" for x in raw[i:i+16]) + ",")
text=f'''#include <lvgl.h>\n\n#ifndef LV_ATTRIBUTE_MEM_ALIGN\n#define LV_ATTRIBUTE_MEM_ALIGN\n#endif\n#ifndef LV_ATTRIBUTE_LARGE_CONST\n#define LV_ATTRIBUTE_LARGE_CONST\n#endif\n\nstatic const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST uint8_t {symbol}_map[] = {{\n''' + "\n".join(lines) + f'''\n}};\n\nconst lv_image_dsc_t {symbol} = {{\n    .header = {{\n        .magic = LV_IMAGE_HEADER_MAGIC,\n        .cf = LV_COLOR_FORMAT_RGB565,\n        .flags = 0,\n        .w = {w},\n        .h = {h},\n        .stride = {w*2},\n        .reserved_2 = 0,\n    }},\n    .data_size = sizeof({symbol}_map),\n    .data = {symbol}_map,\n    .reserved = NULL,\n}};\n'''
Path(output_path).write_text(text, encoding="utf-8")
print(f"generated {output_path}: {w}x{h}, {len(raw)} bytes")
