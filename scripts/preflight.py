#!/usr/bin/env python3
from pathlib import Path
import re
import sys

if len(sys.argv) != 2:
    raise SystemExit("usage: preflight.py <xiaozhi-source-root>")

root = Path(sys.argv[1]).resolve()
main = root / "main"

required_files = [
    main / "display/stellar_max35/stellar_max35_display.cc",
    main / "display/stellar_max35/ui_home.cc",
    main / "display/stellar_max35/ui_chat.cc",
    main / "display/stellar_max35/ui_camera.cc",
    main / "mcp_server.cc",
    root / "sdkconfig.defaults",
]

for p in required_files:
    if not p.exists():
        raise SystemExit(f"missing required file: {p}")
    print("[OK]", p)

defaults = (root / "sdkconfig.defaults").read_text(encoding="utf-8", errors="ignore")
for token in [
    "CONFIG_BOARD_TYPE_SPOTPEAR_ESP32_S3_3_5_LCD=y",
    "CONFIG_USE_WECHAT_MESSAGE_STYLE=n",
    "CONFIG_CAMERA_GC0308=y",
]:
    if token not in defaults:
        raise SystemExit(f"sdkconfig.defaults missing: {token}")
    print("[OK]", token)

cmake = (main / "CMakeLists.txt").read_text(encoding="utf-8", errors="ignore")
for name in [
    "stellar_max35_display.cc",
    "ui_home.cc",
    "ui_chat.cc",
    "ui_camera.cc",
    "camera_gate.cc",
    "stellar_todo.cc",
    "product_tools.cc",
]:
    if name not in cmake:
        raise SystemExit(f"CMake missing product source: {name}")
print("[OK] all product sources are compiled")

board_key = "CONFIG_BOARD_TYPE_SPOTPEAR_ESP32_S3_3_5_LCD"
m = re.search(
    rf"(?m)^\s*(?:if|elseif)\(\s*{re.escape(board_key)}\s*\)\s*$",
    cmake,
)
if not m:
    raise SystemExit("exact MAX35 CMake block missing")
print("[OK] exact MAX35 CMake block present")

board_hits = []
for p in (main / "boards").rglob("*"):
    if p.suffix.lower() not in {".h", ".hpp", ".cc", ".cpp"}:
        continue
    t = p.read_text(encoding="utf-8", errors="ignore")
    if "StellarMax35Display" in t:
        board_hits.append(p)

if not board_hits:
    raise SystemExit("MAX35 board was not attached to StellarMax35Display")
print("[OK] product display attached:", ", ".join(str(p) for p in board_hits))

mcp = (main / "mcp_server.cc").read_text(encoding="utf-8", errors="ignore")
if "stellar_max35::WaitForCameraConsent" not in mcp:
    raise SystemExit("camera shutter gate missing")
print("[OK] camera shutter gate present")

print("[done] preflight passed")
