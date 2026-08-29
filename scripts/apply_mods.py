#!/usr/bin/env python3
from pathlib import Path
import re
import shutil
import sys

if len(sys.argv) != 2:
    raise SystemExit("usage: apply_mods.py <xiaozhi-source-root>")

repo = Path(__file__).resolve().parents[1]
root = Path(sys.argv[1]).resolve()
main = root / "main"
cmake_path = main / "CMakeLists.txt"

if not cmake_path.exists():
    raise SystemExit(f"invalid Xiaozhi source root: {root}")

BOARD_CONFIG = "CONFIG_BOARD_TYPE_SPOTPEAR_ESP32_S3_3_5_LCD"


def function_close(text: str, signature_pos: int) -> int:
    brace = text.find("{", signature_pos)
    if brace < 0:
        raise SystemExit("function opening brace not found")
    depth = 0
    for i in range(brace, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return i
    raise SystemExit("function closing brace not found")


def get_exact_board_dir(cmake_text: str) -> Path:
    # Same philosophy as STELLAR V7.1: inspect only the exact board block.
    m = re.search(
        rf"(?m)^\s*(?:if|elseif)\(\s*{re.escape(BOARD_CONFIG)}\s*\)\s*$",
        cmake_text,
    )
    if not m:
        raise SystemExit(
            f"Exact MAX35 CMake block not found: {BOARD_CONFIG}"
        )

    rest = cmake_text[m.end():]
    next_m = re.search(r"(?m)^\s*(?:elseif\(|else(?:\(|\s*$)|endif\()", rest)
    block_end = m.end() + (next_m.start() if next_m else len(rest))
    block = cmake_text[m.start():block_end]

    d = re.search(r'set\(\s*BOARD_DIR\s+"([^"]+)"\s*\)', block)
    if not d:
        raise SystemExit("MAX35 CMake block has no BOARD_DIR")

    rel = Path(d.group(1))
    board_dir = main / "boards" / rel
    if not board_dir.is_dir():
        raise SystemExit(f"MAX35 BOARD_DIR does not exist: {board_dir}")

    print("[board] exact CMake mapping:", rel)
    return board_dir


# ---------------------------------------------------------------------------
# 1. Copy only our product display layer.
# ---------------------------------------------------------------------------
overlay_src = repo / "overlay/main/display/stellar_max35"
overlay_dst = main / "display/stellar_max35"

if not overlay_src.is_dir():
    raise SystemExit(f"overlay missing: {overlay_src}")

if overlay_dst.exists():
    shutil.rmtree(overlay_dst)
shutil.copytree(overlay_src, overlay_dst)
print("[copy]", overlay_dst)

# ---------------------------------------------------------------------------
# 2. Add product sources to main/CMakeLists.txt using the same stable anchor
#    style as the proven STELLAR project.
# ---------------------------------------------------------------------------
cmake = cmake_path.read_text(encoding="utf-8", errors="ignore")

sources = [
    "display/stellar_max35/stellar_max35_display.cc",
    "display/stellar_max35/ui_home.cc",
    "display/stellar_max35/ui_chat.cc",
    "display/stellar_max35/ui_camera.cc",
    "display/stellar_max35/ui_character.c",
    "display/stellar_max35/camera_gate.cc",
    "display/stellar_max35/stellar_todo.cc",
    "display/stellar_max35/product_tools.cc",
]

missing = [s for s in sources if f'"{s}"' not in cmake]
if missing:
    anchor = '"display/lcd_display.cc"'
    if anchor not in cmake:
        raise SystemExit(
            "Upstream CMake changed: stable display/lcd_display.cc anchor not found"
        )
    addition = "\n".join(f'            "{s}"' for s in missing)
    cmake = cmake.replace(anchor, anchor + "\n" + addition, 1)
    cmake_path.write_text(cmake, encoding="utf-8")
    print("[patch] main/CMakeLists.txt -> product UI sources")

# Resolve board from exact CMake block, not hardware keyword scoring.
board_dir = get_exact_board_dir(cmake)
board_files = sorted(
    p for p in board_dir.rglob("*")
    if p.suffix.lower() in {".h", ".hpp", ".cc", ".cpp"}
)
if not board_files:
    raise SystemExit(f"MAX35 board source files missing: {board_dir}")

display_include = '#include "display/stellar_max35/stellar_max35_display.h"'
tools_include = '#include "display/stellar_max35/product_tools.h"'

# ---------------------------------------------------------------------------
# 3. Attach StellarMax35Display while preserving SpotPear's board hardware.
# ---------------------------------------------------------------------------
patched_display = False
registered_tools = False

for f in board_files:
    text = f.read_text(encoding="utf-8", errors="ignore")
    original = text

    relevant = (
        "SpiLcdDisplay" in text
        or "LcdDisplay" in text
        or "InitializeTools" in text
        or "StellarMax35Display" in text
    )
    if not relevant:
        continue

    # Add include only to files that actually need the type.
    if ("SpiLcdDisplay" in text or "LcdDisplay" in text or "StellarMax35Display" in text):
        if display_include not in text:
            include_anchor = '#include "display/lcd_display.h"'
            if include_anchor in text:
                text = text.replace(
                    include_anchor,
                    include_anchor + "\n" + display_include,
                    1,
                )
            elif f.suffix.lower() in {".cc", ".cpp", ".h", ".hpp"}:
                text = display_include + "\n" + text

    if "InitializeTools" in text and tools_include not in text:
        text = tools_include + "\n" + text

    # Direct construction.
    text, n1 = re.subn(
        r"new\s+SpiLcdDisplay\s*\(",
        "new StellarMax35Display(",
        text,
        count=1,
    )
    if n1:
        patched_display = True
        print("[patch] direct SpiLcdDisplay -> StellarMax35Display:", f)

    # Board-local subclass: preserve all MAX35 behavior; only change its display base.
    classes = re.findall(
        r"class\s+([A-Za-z_][A-Za-z0-9_]*)\s*:\s*public\s+SpiLcdDisplay",
        text,
    )
    for cls in classes:
        text = re.sub(
            rf"class\s+{re.escape(cls)}\s*:\s*public\s+SpiLcdDisplay",
            f"class {cls} : public StellarMax35Display",
            text,
            count=1,
        )
        text = text.replace(
            "using SpiLcdDisplay::SpiLcdDisplay;",
            "using StellarMax35Display::StellarMax35Display;",
        )
        text = re.sub(
            r":\s*SpiLcdDisplay\s*\(",
            ": StellarMax35Display(",
            text,
            count=1,
        )
        text = text.replace(
            "SpiLcdDisplay::SetupUI();",
            "StellarMax35Display::SetupUI();",
        )
        patched_display = True
        print(f"[patch] preserve {cls}, base -> StellarMax35Display:", f)

    # Register memo/weather/product MCP tools in the board, which is the upstream pattern.
    if "stellar_max35::RegisterProductTools();" in text:
        registered_tools = True
    elif "InitializeTools" in text:
        m = re.search(r"void\s+InitializeTools\s*\([^)]*\)\s*\{", text)
        if m:
            close = function_close(text, m.start())
            text = (
                text[:close]
                + "        stellar_max35::RegisterProductTools();\n"
                + text[close:]
            )
            registered_tools = True
            print("[patch] RegisterProductTools -> InitializeTools:", f)

    if text != original:
        f.write_text(text, encoding="utf-8")

if not patched_display:
    raise SystemExit(
        "Exact MAX35 board was found, but no SpiLcdDisplay construction/subclass "
        "was found in that board. This is a real upstream API change, not a board guess."
    )

# ---------------------------------------------------------------------------
# 4. Camera: retain Xiaozhi's native Capture()/Explain() implementation and
#    insert only the touch shutter confirmation before Capture().
# ---------------------------------------------------------------------------
mcp = main / "mcp_server.cc"
if not mcp.exists():
    raise SystemExit("main/mcp_server.cc missing")

text = mcp.read_text(encoding="utf-8", errors="ignore")

gate_include = '#include "display/stellar_max35/camera_gate.h"'
if gate_include not in text:
    first_include = text.find("#include")
    eol = text.find("\n", first_include)
    if first_include < 0 or eol < 0:
        raise SystemExit("mcp_server.cc include block not found")
    text = text[:eol + 1] + gate_include + "\n" + text[eol + 1:]

if not registered_tools:
    if tools_include not in text:
        first_include = text.find("#include")
        eol = text.find("\n", first_include)
        text = text[:eol + 1] + tools_include + "\n" + text[eol + 1:]

if "stellar_max35::WaitForCameraConsent" not in text:
    capture = re.search(
        r"if\s*\(\s*!\s*camera->Capture\s*\(\s*\)\s*\)\s*\{",
        text,
    )
    if not capture:
        raise SystemExit(
            "Native self.camera.take_photo Capture() anchor not found. "
            "Vendor camera API changed."
        )

    gate = (
        'if (!stellar_max35::WaitForCameraConsent('
        'properties["question"].value<std::string>(), 30000)) {\n'
        '                    throw std::runtime_error("Camera capture cancelled or timed out");\n'
        '                }\n                '
    )
    text = text[:capture.start()] + gate + text[capture.start():]
    print("[patch] touch shutter gate -> native camera Capture()")

if not registered_tools and "stellar_max35::RegisterProductTools();" not in text:
    restore = "// Restore the original tools list"
    if restore not in text:
        raise SystemExit(
            "Board has no InitializeTools() and MCP fallback anchor changed"
        )
    text = text.replace(
        restore,
        "stellar_max35::RegisterProductTools();\n\n    " + restore,
        1,
    )
    print("[patch] RegisterProductTools -> MCP fallback")

mcp.write_text(text, encoding="utf-8")

print("[done] deterministic MAX35 product patch applied")
print("[done] hardware board kept vendor-owned:", board_dir)
