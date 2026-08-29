# 三页面 UI 替换契约

后续做视觉优化时，默认只替换下面一个文件：

| 页面 | 文件 | 不应承担的职责 |
|---|---|---|
| 桌面 | `ui_home.cc` | 不直接操作小智会话状态、不直接读 GPIO |
| 对话 | `ui_chat.cc` | 不调用网络/TTS、不处理相机 |
| 拍照 | `ui_camera.cc` | 不直接调用 `camera->Capture()`，只发出快门事件 |

## 必须保留的函数签名

以 `ui_pages.h` 为准。只要这些接口不变，`stellar_max35_display.cc` 就无需跟着视觉版本变化。

## 资源替换

角色资源：

```text
assets/character_source.png
```

编译资源：

```text
overlay/main/display/stellar_max35/ui_character.c
```

若替换原图，可用：

```bash
python scripts/image_to_lvgl_rgb565.py \
  assets/character_source.png \
  overlay/main/display/stellar_max35/ui_character.c \
  stellar_max35_character
```

建议角色图继续控制在约 188×276 左右，避免无意义增加 Flash 占用。

## 3.5 寸触摸设计约束

- 主要点击目标建议不小于约 60 px；拍照页快门当前为 72×60 px。
- 长回答优先给文字空间，不在 Chat 页面堆常驻返回/拍照按钮。
- 桌面的时间、日期、天气属于“环境信息”，不要重新包成三个白色卡片。
- 备忘录可以保留一块较大的半透明纸张感区域，因为它是桌面的核心可读内容。
- 新皮肤不要依赖 480×320 以外的隐藏滚动区域。
