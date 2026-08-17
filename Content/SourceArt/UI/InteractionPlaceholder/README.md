# 交互占位美术源资产

本目录来自人工交付的 `正式-交互占位.zip`，于 2026-08-17 按 Plan 45 接入。原始 ZIP 不进入仓库；`_SourceManifest.csv` 记录每个交付文件的原路径、分类、尺寸、目标相对路径和 SHA-256。

## 目录语义

- `References/`：整屏效果图与标注/说明图，仅用于 WBP 布局和视觉对照，不直接导入为正常运行时整屏纹理。
- `Elements/`：可评审的原始 PNG 切图。只有被现有 WBP 实际消费的元素才导入 `/Game/ReEcho/Textures/UI/InteractionPlaceholder/**`。
- `Fonts/PendingLicense/`：交付包内的占位 TTF 及说明。当前未附授权/来源证明，TTF 仅留在本地隔离目录并由 `.gitignore` 排除，不导入、不在 WBP 中引用、不提交分发；清单仍保留其来源哈希以便授权后复核。

## 导入边界

- 保留源文件的中文交付名，运行时 Unreal 资产使用稳定 ASCII 名称。
- 导入、移动、重命名和保存 `.uasset` 只通过 Unreal Editor 或可复现 Editor 脚本执行。
- 导入 UI 纹理需核对 sRGB、透明通道、无 mipmap、UI 压缩和双线性过滤。
- 效果图中展示但当前产品流程不存在的“存档回溯”和“关于我们”只作参考；资产图片不自动授权新页面或玩法行为。
