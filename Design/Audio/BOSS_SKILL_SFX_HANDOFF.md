# Boss 技能音效 — 程序交接说明

- 日期：2026-08-26
- 策划：程乐兮（Designer）
- 交付形态：资源已入仓库 + 本说明 + 配套 `ECHO_Boss技能音效_程序交接.xlsx`

## 一、需求摘要

给 Boss（绵羊）两个技能新增独立音效，其余技能保持现状（继续回退到 `Boss.Attack` 兜底音效）：

1. 祷告光束 `M_SHEEP_PrayerBeam`：光束出现时播「冲击波光束.mp3」。
2. 闪身打击 `M_SHEEP_BlinkSlam`：BOSS 消失 / 出现 / 砸地三个时刻分别播「闪身打击_消失 / 闪身打击_出现 / 闪身打击_砸击」。
3. 其他技能（`M_SHEEP_MeleeSweep` / `M_SHEEP_StationaryVolley` / `M_SHEEP_MovingSpread`）不用新音效。

## 二、音效映射

| 技能 | 触发时机 | 音效文件 | EventId / VariantId | 现有代码触发点 | 是否动代码 |
|---|---|---|---|---|---|
| 祷告光束 | 光束出现（伤害窗口开始） | 冲击波光束.mp3 | `Boss.Attack` / `M_SHEEP_PrayerBeam` | 已有，ReEchoEnemyActor.cpp:1476 | 否 |
| 闪身打击 | Boss 砸地（落点结算） | 闪身打击_砸击.mp3 | `Boss.Attack` / `M_SHEEP_BlinkSlam` | 已有，ReEchoEnemyActor.cpp:1547 | 否 |
| 闪身打击 | Boss 消失（传送前） | 闪身打击_消失.mp3 | `Boss.Attack` / `M_SHEEP_BlinkSlam_Vanish` | 无，需新增 | **是** |
| 闪身打击 | Boss 出现（传送后） | 闪身打击_出现.mp3 | `Boss.Attack` / `M_SHEEP_BlinkSlam_Appear` | 无，需新增 | **是** |

## 三、代码改动（仅 2 处新增触发点）

文件：`Source/ReEcho/Private/Graybox/ReEchoEnemyActor.cpp`
函数：`AReEchoEnemyActor::ApplyBossIntent`，`M_SHEEP_BlinkSlam` 分支（约第 1456-1472 行）。

```cpp
if (Intent.bRequestTeleport && !Intent.TeleportDestination.IsNearlyZero())
{
    // 消失音效（传送前，当前位置）
    CombatAudioAdapter->PostConfiguredAttack(GetActorLocation(), TEXT("M_SHEEP_BlinkSlam_Vanish"));
    SetActorLocation(Intent.TeleportDestination, false, nullptr, ETeleportType::TeleportPhysics);
    // 出现音效（传送后，目标位置）
    CombatAudioAdapter->PostConfiguredAttack(Intent.TeleportDestination, TEXT("M_SHEEP_BlinkSlam_Appear"));
}
```

接口：`void PostConfiguredAttack(const FVector& WorldLocation, FName VariantId = NAME_None) const;`

设计说明：复用 `Boss.Attack` 事件 + 合成 VariantId 字符串，不新增 EventId 常量（遵循 `ReEchoAudioEvents.h` 头部注释）。
`M_SHEEP_BlinkSlam_Vanish` / `M_SHEEP_BlinkSlam_Appear` 仅为 VariantId 字符串，只需与 `audio_events.csv` 的 VariantId 列一致。

## 四、数据改动

`Design/Data/ReEchoAudioEvents.xlsx`（源头）→ sync 生成 `Content/Data/audio_events.csv`，新增 4 行变体（参数沿用现有 `Boss.Attack` 无变体兜底行；当前表已为 16 列，末列 `StartTimeSeconds` 填 0）：

```csv
Boss.Attack,M_SHEEP_PrayerBeam,<AssetPath>,CombatSfx,OneShot,true,1,0.95,1.05,0.05,8,20,PauseWithGame,200,2000,0
Boss.Attack,M_SHEEP_BlinkSlam,<AssetPath>,CombatSfx,OneShot,true,1,0.95,1.05,0.05,8,20,PauseWithGame,200,2000,0
Boss.Attack,M_SHEEP_BlinkSlam_Vanish,<AssetPath>,CombatSfx,OneShot,true,1,0.95,1.05,0.05,8,20,PauseWithGame,200,2000,0
Boss.Attack,M_SHEEP_BlinkSlam_Appear,<AssetPath>,CombatSfx,OneShot,true,1,0.95,1.05,0.05,8,20,PauseWithGame,200,2000,0
```

## 五、导入脚本改动

`scripts/audio/import_audio_catalog_assets.py` 的 `VARIANT_ONE_SHOTS` 字典新增 4 条（源路径相对 `Design/Audio`，惯例导入 `Derived` 的 mono PCM16 WAV）：

```python
"Boss.Attack/M_SHEEP_PrayerBeam":      ("Derived/Variants/BossAttack/冲击波光束.wav",  "/Game/ReEcho/Audio/Variants/BossAttack", "Boss_Attack_M_SHEEP_PrayerBeam"),
"Boss.Attack/M_SHEEP_BlinkSlam":       ("Derived/Variants/BossAttack/闪身打击_砸击.wav", "/Game/ReEcho/Audio/Variants/BossAttack", "Boss_Attack_M_SHEEP_BlinkSlam"),
"Boss.Attack/M_SHEEP_BlinkSlam_Vanish":("Derived/Variants/BossAttack/闪身打击_消失.wav", "/Game/ReEcho/Audio/Variants/BossAttack", "Boss_Attack_M_SHEEP_BlinkSlam_Vanish"),
"Boss.Attack/M_SHEEP_BlinkSlam_Appear":("Derived/Variants/BossAttack/闪身打击_出现.wav", "/Game/ReEcho/Audio/Variants/BossAttack", "Boss_Attack_M_SHEEP_BlinkSlam_Appear"),
```

源 mp3 需先经 `prepare_formal_audio.py` 转 mono PCM16 WAV 落到 `Derived/Variants/BossAttack/`，再导入。

## 六、资源清单与溯源

目录：`Design/Audio/Source/Formal/Variants/BossAttack/`

| 文件 | SHA-256 |
|---|---|
| 冲击波光束.mp3 | `faddb22535a999b085e572d02c6862baa393f7751388b0b5f3992f2dd1a4ff24` |
| 闪身打击_砸击.mp3 | `c5b345b9ca21c52522c721f227abc65865eefd36a4807f3f62ead4e86ab35d0f` |
| 闪身打击_消失.mp3 | `b963f5aef2a2070528b4d7dcf4e05aa5e8f4dd0133ad9654bf1c8a2a47aa48b3` |
| 闪身打击_出现.mp3 | `9aac51c9aac93cbe3721fea4b013f82c00f98d6dab33dd6181c218207f2a3b71` |

来源：企业微信微盘共享空间《【开普勒】FIFA足队-共享空间》/ 音效 / 音效 目录，策划程乐兮 2026-08-26 提供。
