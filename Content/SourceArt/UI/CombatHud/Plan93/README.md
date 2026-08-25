# Plan93 战斗 HUD 源资产

本目录归档用户为 Plan93 提供的“正式-UI视觉/战斗场景”交付。`References/1-战斗场景.png` 是 1920×1080 构图参考，只用于 WBP 对照；`Elements/` 保留 9 张透明 PNG 原始切图。`技能栏.png` 在 Plan93 被判定为废案，`时间显示.png` 的倒计时黑底在 Plan102 被判定为废案，两者仅归档、不导入运行时，其余 7 张用于正式 HUD。

- 正式运行时 Texture2D 使用 `_SourceManifest.csv` 中的稳定 ASCII 名，导入 `/Game/ReEcho/Textures/UI/CombatHud/`；废案行的 RuntimeName 留空。
- 不把整屏 Reference 导入运行时，不从图中推导新的玩法或输入行为。
- WBP 负责布局、锚点、尺寸和表现；C++ 只提供生命、时间碎片、关卡、倒计时和小地图等真实只读状态。
- 原始中文文件名、尺寸、字节数与 SHA-256 保留在清单中，便于后续复核和重导。
