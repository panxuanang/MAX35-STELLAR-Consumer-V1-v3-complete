# 第一次 GitHub Actions 编译

## 1. 上传仓库

请确认这些路径都上传了：

```text
.github/workflows/build-max35.yml
overlay/main/display/stellar_max35/
scripts/
VERSION
```

GitHub 网页上传时不要遗漏 `.github`，否则不会出现 Workflow。

## 2. 运行 Workflow

进入：

`Actions -> Build MAX35 STELLAR Consumer V1 -> Run workflow`

Workflow 使用 `espressif/idf:v5.5`，与 SpotPear 当前 MAX35 教程所要求的 ESP-IDF 5.5 对齐。

默认从 SpotPear 教程所指向的 Google Drive 文件下载 `xiaozhi-esp32-3.1.0`。

## 3. 如果 Google Drive 下载受限

在仓库：

`Settings -> Secrets and variables -> Actions -> Secrets`

新增：

```text
XIAOZHI_SOURCE_URL
```

值填写你自己可直接下载的 SpotPear `xiaozhi-esp32-3.1.0.zip` 地址。Workflow 会优先使用这个地址。

不要把需要登录的私人下载 token 直接写进 YAML。

## 4. 如果自动识别到了错误板目录

构建日志中会打印：

```text
[board] selected: ...
[board] top candidates: ...
```

在仓库：

`Settings -> Secrets and variables -> Actions -> Variables`

新增：

```text
MAX35_BOARD_DIR
```

填 `main/boards` 下的相对路径即可，例如（仅示例，以实际日志为准）：

```text
spotpear/esp32-s3-max35
```

不要在没有日志证据时猜路径。

## 5. 成功后拿哪个 BIN

Artifact 中：

```text
MAX35_STELLAR_<version>_merged.bin
```

是合并后的完整烧录镜像。`components/` 目录保留 ESP-IDF 生成的分区镜像，适合调试和恢复。

## 6. 第一次实机重点验证

按下面顺序测，能最快定位问题：

1. 开机 / 配网 / 激活是否仍正常；
2. 扬声器与麦克风；
3. 横屏方向、触摸坐标是否一致；
4. 桌面时间与备忘录；
5. 唤醒后是否自动进入 Chat；
6. 长回答滚动与自动回桌面；
7. 说“拍照搜题”后是否进入 Camera；
8. 点击快门后是否真正 Capture、识别并朗读；
9. 重启后 NVS 备忘录是否仍在。

## 7. 当前 V1 的已知集成边界

本仓库在生成时无法在本地拿到并完整编译 SpotPear Google Drive 源码，所以使用了结构检查和模拟 vendor tree 测试补丁逻辑；真正的厂家源码 + ESP-IDF 5.5 编译由 GitHub Actions 执行。

因此第一次 CI 如果遇到 **SpotPear 厂家源码内部文件名/类名与当前公开小智结构不同**，不要改硬件 GPIO。保留失败日志即可，优先只修 `scripts/apply_mods.py` 或对应的一个 UI 文件。
