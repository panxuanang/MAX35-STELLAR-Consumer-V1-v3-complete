#!/usr/bin/env python3
from pathlib import Path
import re
import shutil
import sys

if len(sys.argv) != 2:
    raise SystemExit("usage: configure_max35.py <xiaozhi-source-root>")

root = Path(sys.argv[1]).resolve()
main = root / "main"
kconfig = main / "Kconfig.projbuild"

if not kconfig.exists():
    raise SystemExit(f"missing {kconfig}")

# This exact board symbol is confirmed by the real SpotPear source downloaded in CI.
BOARD_SYMBOL = "BOARD_TYPE_SPOTPEAR_ESP32_S3_3_5_LCD"
BOARD_CONFIG = f"CONFIG_{BOARD_SYMBOL}=y"

ktext = kconfig.read_text(encoding="utf-8", errors="ignore")
if not re.search(rf"(?m)^\s*config\s+{re.escape(BOARD_SYMBOL)}\s*$", ktext):
    raise SystemExit(
        f"SpotPear MAX35 board symbol not found: {BOARD_SYMBOL}\n"
        "The vendor source package changed; do not guess another board automatically."
    )

# Keep this list explicit and deterministic.
# Camera symbols cover the two camera stacks commonly used by Xiaozhi/SpotPear:
#   - esp-video-components: CONFIG_CAMERA_GC0308...
#   - esp32-camera: CONFIG_GC0308_SUPPORT
# Unknown sdkconfig symbols are ignored by Kconfig; recognized ones are applied.
managed = [
    BOARD_CONFIG,

    # Product UI: use our own page layout, not upstream WeChat bubbles.
    "CONFIG_USE_DEFAULT_MESSAGE_STYLE=y",
    "CONFIG_USE_WECHAT_MESSAGE_STYLE=n",

    # MAX35 is 16 MB Flash.
    "CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y",

    # Clock typography used by ui_home.cc.
    "CONFIG_LV_FONT_MONTSERRAT_48=y",

    # SpotPear rear-camera SKU: GC0308.
    # esp-video-components names:
    "CONFIG_CAMERA_GC0308=y",
    "CONFIG_CAMERA_GC0308_AUTO_DETECT_DVP_INTERFACE_SENSOR=y",
    "CONFIG_CAM_CTRL_DVP_ENABLE=y",
    "CONFIG_CAMERA_GC0308_DVP_YUV422_YUYV_640X480_16FPS=y",
    "CONFIG_CAMERA_GC0308_DVP_DEFAULT_FMT_YUV422_YUYV_640X480_16FPS=y",

    # esp32-camera compatibility name:
    "CONFIG_GC0308_SUPPORT=y",
]

defaults = root / "sdkconfig.defaults"
old = defaults.read_text(encoding="utf-8", errors="ignore").splitlines() if defaults.exists() else []

# Remove only settings we intentionally own.
managed_prefixes = (
    "CONFIG_BOARD_TYPE_",
    "CONFIG_USE_DEFAULT_MESSAGE_STYLE=",
    "CONFIG_USE_WECHAT_MESSAGE_STYLE=",
    "CONFIG_LV_FONT_MONTSERRAT_48=",
    "CONFIG_ESPTOOLPY_FLASHSIZE_",
    "CONFIG_CAMERA_GC0308",
    "CONFIG_CAM_CTRL_DVP_ENABLE=",
    "CONFIG_GC0308_SUPPORT=",
)

clean = []
for line in old:
    s = line.strip()
    if any(s.startswith(prefix) for prefix in managed_prefixes):
        continue
    if s == "# CONFIG_USE_WECHAT_MESSAGE_STYLE is not set":
        continue
    clean.append(line)

clean += [
    "",
    "# --- STELLAR MAX35 deterministic settings ---",
    *managed,
]

defaults.write_text("\n".join(clean).rstrip() + "\n", encoding="utf-8")

# Generated sdkconfig overrides defaults, so start clean just like a fresh menuconfig.
sdkconfig = root / "sdkconfig"
if sdkconfig.exists():
    backup = root / "sdkconfig.vendor.backup"
    shutil.copy2(sdkconfig, backup)
    sdkconfig.unlink()
    print(f"[backup] existing sdkconfig -> {backup}")

print("[ok] deterministic MAX35 configuration written")
for item in managed:
    print("  ", item)
