# 武器攻击音效 +20% 增益 —— UE 资产层交接说明

> 角色：策划（程乐兮） ｜ 状态：待 UE 操作者（程序 / 音频）执行 ｜ 数据层不涉及改动

## 背景与结论

玩家反馈：武器攻击音效音量过低，要求增加 20%。

经核实，武器音效在数据层（`audio_events.csv` 的 `BaseVolume` 字段）**已满格 = 1.0（100%）**。数据契约（`scripts/validate_project.py`）与 C++（`Source/ReEchoAudio/Private/ReEchoAudioCatalog.cpp`）均硬性限制音量 `[0, 1]`，任何事件音量都不能超过 1.0，因此「+20%」无法通过数据层表达。

**决策（策划确认）**：改在 **UE 资产层**对武器攻击音效做 **+20% 增益**，数据层 `BaseVolume` 保持 1.0 不变。

## 目标

对下列 5 个 SoundWave 资产做 **+20% 幅度增益（即 ×1.2，约 +1.58 dB）**：

| # | EventId | VariantId | 资产路径（Content 内相对路径） |
|---|---------|-----------|-------------------------------|
| 1 | Combat.Attack | （兜底） | `/Game/ReEcho/Audio/Combat/Combat_Attack` |
| 2 | Combat.Attack | W_J_01 | `/Game/ReEcho/Audio/Variants/CombatAttack/Combat_Attack_W_J_01` |
| 3 | Combat.Attack | W_J_04 | `/Game/ReEcho/Audio/Variants/CombatAttack/Combat_Attack_W_J_04` |
| 4 | Combat.Attack | W_J_08 | `/Game/ReEcho/Audio/Variants/CombatAttack/Combat_Attack_W_J_08` |
| 5 | Combat.Attack | W_J_09 | `/Game/ReEcho/Audio/Variants/CombatAttack/Combat_Attack_W_J_09` |

## 实施建议（供 UE 操作者参考，任选其一）

1. **首选 — 资产重新导入前做增益**：在音频编辑器（Audacity / DAW）中对源音频做 +1.58 dB 增益后，重新导入对应 SoundWave 资产。
2. **SoundCue 包裹**：用 SoundCue + Volume 调制节点（×1.2）包裹上述 SoundWave，数据表 `AssetPath` 指向新 SoundCue（若采用此方案需同步改 `ReEchoAudioEvents.xlsx` 的 `AssetPath` 并重新 sync）。
3. **Sound Class / Submix 增益**：若这些武器音效走独立的 Sound Class / Submix，可在其上做 +1.58 dB 增益。

## 不做的事

- 不改 `ReEchoAudioEvents.xlsx` 的 `BaseVolume`（保持 1.0）。
- 不改 `validate_project.py` / `ReEchoAudioCatalog.cpp` 的音量上限。

## 关联已完成改动

- 战斗 BGM 降 50%（`Music.Encounter` 兜底 + `Stage.1/2/3`、`Music.Boss` 的 `BaseVolume` 1.0 → 0.5）此前以 `[DESIGNER] d0d5496f` 提交，但**未被程序合入 main**；程序随后重构音频表为 16 列（新增 `StartTimeSeconds`），当前 `BaseVolume` 仍 = 1.0，若仍需降 BGM 须在新结构上重做。
