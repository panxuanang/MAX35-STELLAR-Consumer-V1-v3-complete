# Architecture

## 状态流

```text
HOME
  |  唤醒 / Listening / 用户语音
  v
CHAT  <-------------------------------+
  |                                    |
  | AI 调用 self.camera.take_photo     | Capture + Explain result
  v                                    |
CAMERA -- 用户点击大快门 --------------+

CHAT -- 语音播放结束 + 文本阅读完成 --> HOME
```

## 分层

```text
ui_home.cc / ui_chat.cc / ui_camera.cc
              |
              v
stellar_max35_display.cc    <- 只做产品页面状态机
              |
              v
LcdDisplay / SpiLcdDisplay  <- 小智显示框架
              |
              v
SpotPear MAX35 board        <- LCD / touch / camera / audio / PMIC
```

### 为什么不复制整份上游

商业设备后续需要持续跟随小智协议和服务器变化。如果把整个上游 fork 后大改，合并成本会快速上升。本工程采用 overlay + deterministic patch：

1. CI 获取指定厂家源码；
2. 自动识别 MAX35 板型 Kconfig；
3. 复制产品层文件；
4. 只把 MAX35 的显示基类替换为 `StellarMax35Display`；
5. 在原生 camera tool 的 `Capture()` 前插入触摸快门确认；
6. 编译和合并 BIN。

因此硬件初始化仍由 SpotPear 维护。

## 页面替换约束

视觉迭代时，优先只改以下文件之一：

- `ui_home.cc`
- `ui_chat.cc`
- `ui_camera.cc`

只要不修改 `ui_pages.h` 的函数签名，就不需要改产品控制器。

## 拍照隐私/交互

AI 调用相机工具后不会立刻拍照。`camera_gate.cc` 会让产品显示进入 Camera 页面并等待触摸快门。超时则取消本次拍照工具调用。

这同时解决两个消费级问题：

- 用户知道设备什么时候准备拍摄；
- 云端 AI 不能在没有本机触摸确认的情况下完成该次拍照。

V1 超时默认由补丁设为 30 秒，可在 `scripts/apply_mods.py` 调整。
