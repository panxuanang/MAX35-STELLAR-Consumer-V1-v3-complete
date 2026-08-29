# MAX35 STELLAR Consumer V1

面向 **SpotPear ESP32S3-MAX35-TouchLCD-BCamera-Case** 的小智消费级 UI 覆盖工程。

本仓库不是重新维护一整份小智源码。GitHub Actions 会下载 SpotPear 官方 MAX35 小智源码，保留厂家的 LCD / GT911 / GC0308 / ES8311 / 电源管理等板级实现，只覆盖产品 UI、备忘录 MCP 与拍照搜题交互层，再编译出可烧录 BIN。

## 设计目标

- 横屏 `480 x 320`，针对 3.5 英寸触摸屏重新控制信息密度。
- 桌面：浅色动漫视觉；时间 / 日期 / 星期 / 天气直接融入背景；大尺寸备忘录自然融入画面；不放“按住说话”等常驻按钮。
- 对话：像 STELLAR 一样由语音状态自动进入；回答区域占据几乎全屏；长文本单向慢滚动；回答完成后自动回桌面。
- 拍照搜题：用户语音说“拍照搜题 / 帮我看这道题”后调用小智内置摄像头工具，进入独立拍照页；只有这里保留一个大尺寸触摸快门；点击后继续走厂家/小智原有 `Capture -> Explain -> TTS` 链路。
- 三个视觉页面完全拆开，后续改视觉时优先只替换一个文件。

## 最重要的三个 UI 文件

```text
overlay/main/display/stellar_max35/
├── ui_home.cc       # 桌面
├── ui_chat.cc       # 对话
└── ui_camera.cc     # 拍照搜题
```

页面接口定义在 `ui_pages.h`，页面切换和小智状态机适配集中在 `stellar_max35_display.cc`。不要把业务状态机重新塞回三个 UI 文件，这样后续换皮肤最省事。

## 仓库结构

```text
.github/workflows/build-max35.yml   GitHub Actions 自动编译
assets/                             原始角色素材
preview/                            设计方向预览
scripts/                            下载后定位/配置/补丁/打包脚本
overlay/main/display/stellar_max35/ 产品 UI 与产品工具
VERSION                             产品覆盖层版本
```

## 直接编译

1. 新建一个 GitHub 仓库，把本项目**整个目录内容**上传到仓库根目录，包括隐藏目录 `.github`。
2. 打开 GitHub 仓库的 **Actions**。
3. 选择 **Build MAX35 STELLAR Consumer V1**。
4. 点击 **Run workflow**。
5. 编译成功后，在该次 Workflow 底部下载 `MAX35-STELLAR-Consumer-Firmware`。
6. 优先使用其中的 `MAX35_STELLAR_*_merged.bin` 作为整包烧录文件；`components/` 内同时保留 bootloader / partition / app 等单独 BIN 方便救砖和调试。

更详细见 `docs/first-build.md`。

## “拍照搜题”角色提示建议

在你的小智角色/系统提示词中加入类似规则：

> 设备有后置摄像头。用户说“拍照搜题”“帮我看这道题”“看一下这题怎么做”时，调用 `self.camera.take_photo`。`question` 要明确要求识别题目、给出正确答案和简洁步骤，并使用中文讲解。设备会先进入拍照页面等待用户点击屏幕快门。

这里仍然调用小智原生相机工具，不另造一套图片上传协议。

## 备忘录

保留 STELLAR 思路，提供本地 NVS MCP：

- `self.stellar.todo.list`
- `self.stellar.todo.add`
- `self.stellar.todo.remove`
- `self.stellar.todo.clear`

桌面最多显示前三条，底层最多保存 8 条。当前 V1 的 `when` 是展示文本，不做自动过期删除，避免错误理解自然语言日期。

## 天气

UI 已留出天气区域，但固件不会伪造天气。`self.stellar.weather.set` 用于把可信的天气服务结果写入桌面。你可以在服务端 MCP / 角色工具里获得真实天气后再调用它。

## 消费级工程边界

这个版本刻意不修改厂家 GPIO、摄像头传感器引脚、GT911、LCD 初始化、音频 codec、功放和 PMIC。这样 UI 迭代与硬件 Bring-up 解耦，量产问题更容易定位。

第一版拍照页面也**不假定厂家源码一定有连续实时预览 API**；它先提供取景引导和物理触摸确认，然后仍由原生相机 `Capture()` 拍摄。拿到第一次 MAX35 CI 编译和实机日志后，再把实时预览接入会更稳妥。

## 上游与许可说明

- Xiaozhi 与 SpotPear 厂家源码的版权/许可证继续遵循它们各自的上游条款。
- 本仓库只包含产品覆盖层、构建脚本和本项目视觉资源，不重新分发 SpotPear 的完整源码包。
