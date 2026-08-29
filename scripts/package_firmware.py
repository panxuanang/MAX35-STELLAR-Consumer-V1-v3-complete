#!/usr/bin/env python3
"""Create one merged ESP32-S3 BIN from an ESP-IDF build directory.

Uses flasher_args.json when available because it is less ambiguous than parsing
human-oriented build logs.  Falls back to an existing merged-binary.bin if the
vendor build already produced one.
"""
from pathlib import Path
import json
import shutil
import subprocess
import sys

if len(sys.argv) != 3:
    raise SystemExit("usage: package_firmware.py BUILD_DIR OUTPUT_BIN")

build = Path(sys.argv[1]).resolve()
out = Path(sys.argv[2]).resolve()
out.parent.mkdir(parents=True, exist_ok=True)
if not build.is_dir():
    raise SystemExit(f"build directory not found: {build}")

for existing_name in ("merged-binary.bin", "merged.bin", "firmware-merged.bin"):
    existing = build / existing_name
    if existing.exists():
        shutil.copy2(existing, out)
        print(f"Copied existing merged image: {existing} -> {out}")
        raise SystemExit(0)

args_json = build / "flasher_args.json"
if not args_json.exists():
    raise SystemExit("flasher_args.json not found; cannot safely construct merged image")

cfg = json.loads(args_json.read_text(encoding="utf-8"))
flash_files = cfg.get("flash_files") or {}
if not flash_files:
    raise SystemExit("flasher_args.json contains no flash_files")

extra = cfg.get("extra_esptool_args") or {}
settings = cfg.get("flash_settings") or {}
chip = extra.get("chip") or "esp32s3"

cmd = [sys.executable, "-m", "esptool", "--chip", str(chip), "merge_bin", "-o", str(out)]
for key, flag in (("flash_mode", "--flash_mode"), ("flash_freq", "--flash_freq"), ("flash_size", "--flash_size")):
    value = settings.get(key)
    if value:
        cmd += [flag, str(value)]

# sort offsets numerically so the resulting command is deterministic
for offset, file_name in sorted(flash_files.items(), key=lambda kv: int(kv[0], 0)):
    file_path = Path(file_name)
    if not file_path.is_absolute():
        file_path = build / file_path
    if not file_path.exists():
        raise SystemExit(f"flash file missing: {file_path}")
    cmd += [str(offset), str(file_path)]

print("Merging", len(flash_files), "flash images")
subprocess.run(cmd, check=True, cwd=build)
print("Wrote", out)
