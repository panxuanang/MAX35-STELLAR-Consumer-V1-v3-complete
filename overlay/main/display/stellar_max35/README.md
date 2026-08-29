# UI replacement contract

The product UI is intentionally split into exactly three replaceable visual files:

- `ui_home.cc` — desktop only
- `ui_chat.cc` — full-screen conversation only
- `ui_camera.cc` — camera/shutter page only

`stellar_max35_display.cc` is the state/controller layer. Do not put coordinates, colors or decorative widgets there.
`camera_gate.cc` is the privacy/interaction bridge that makes Xiaozhi's camera MCP wait for a physical touch before capture.
`stellar_todo.cc` is persistent data, independent of the visual page.

This separation means future visual iterations normally replace one `.cc` file and nothing else.
