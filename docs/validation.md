# 交付前验证记录

本仓库生成时完成了以下本地检查：

- `scripts/*.py` 全部通过 Python 语法编译检查；
- `.github/workflows/build-max35.yml` 通过 YAML 解析；
- 使用模拟的 SpotPear MAX35 vendor tree 验证：
  - 能识别真正的 `Spotpear ESP32-S3-3.5-LCD-cam` 板型配置，而不会误选同一 Kconfig 文件中的其他选项；
  - 能锁定 GC0308 / YUV422 / 后摄 90° / Camera Enable；
  - 能保留厂家 `CustomLcdDisplay`，仅把其基类切换为 `StellarMax35Display`；
  - 能同步修正继承构造函数 `using`；
  - 能注册产品 MCP；
  - 能在原生 `camera->Capture()` 前插入触摸快门门控；
  - `preflight.py` 全部通过；
- 合并 BIN 打包脚本的“已有 merged image”路径已验证。

## 仍需第一次 GitHub Actions / 实机确认的项目

当前运行环境没有直接执行 SpotPear Google Drive 厂家源码 + ESP-IDF 5.5 的完整编译，因此以下项目必须以第一次 CI 和实机为准：

1. SpotPear 当前下载包内部文件名/类名是否与公开结构完全一致；
2. MAX35 厂家 Kconfig 的具体 symbol 名称；
3. 480x320 横屏方向与 GT911 触摸坐标；
4. GC0308 的 Capture / Explain 实机链路；
5. 音频全双工、唤醒和页面自动切换时序。

设计原则是：若第一次 CI 因厂家源码结构差异失败，优先只调整 `scripts/apply_mods.py` / `scripts/configure_max35.py`，不要动厂家 GPIO 和硬件初始化。
