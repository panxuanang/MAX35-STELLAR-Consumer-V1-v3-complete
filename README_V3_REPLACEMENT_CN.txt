MAX35-STELLAR Consumer V1 - V3 Complete Replacement

这是完整替换包，不是补丁包。

已经合并：
- 完整 UI / assets / preview / docs
- 修正版 .github/workflows/build-max35.yml
- 修正版 scripts/configure_max35.py
- 修正版 scripts/apply_mods.py
- 修正版 scripts/preflight.py

使用方法：
1. 解压本压缩包。
2. 将文件夹内全部内容上传/覆盖到 GitHub 仓库根目录。
3. 确保 .github/workflows/build-max35.yml 也被上传。
4. Commit changes 后等待新的 GitHub Actions 自动构建。
5. 不要 Re-run 旧的失败任务；使用新 commit 自动产生的新任务。
