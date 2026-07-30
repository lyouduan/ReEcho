# ReEcho GAS 新人上手指南

本文面向第一次接触 Unreal Engine Gameplay Ability System（GAS）的开发者，目标是让新人读懂 ReEcho 当前实现，并安全完成一个属性、效果或技能改动。

## 1. 30 分钟阅读路线

1. 先读本文“核心模型”。
2. 读 `ReEchoCombatAttributeSet.h/.cpp`，认识运行时属性与结算。
3. 读 `ReEchoGameplayEffects.cpp`，理解初始化、伤害、治疗和冷却。
4. 读 `ReEchoPlayerAbilities.cpp`，理解 Commit、AbilityTask 和 EndAbility。
5. 在 `ReEchoPlayerPawn.cpp` 搜索 `InitAbilityActorInfo`、`GrantAbility`、`AbilityInputPressed`。
6. 最后阅读“常见改动”“项目红线”和自动化测试。

## 2. 核心模型

| 部件 | 职责 | ReEcho 对应实现 |
| --- | --- | --- |
| Ability System Component（ASC） | 保存技能、标签、效果与属性聚合结果 | 玩家和敌人 Actor 上的 `UAbilitySystemComponent` |
| Attribute Set | 声明属性，集中处理钳制与结算 | `UReEchoCombatAttributeSet` |
| Gameplay Effect（GE） | 修改属性、授予状态标签、表达冷却 | `ReEchoGameplayEffects.*` |
| Gameplay Ability（GA） | 判断能否执行、提交消耗并管理技能流程 | `ReEchoPlayerAbilities.*` |
| Gameplay Tag | 标识输入、技能、状态、冷却和动态数据 | `ReEchoGameplayTags.*` |
| Ability Task | 表达等待输入、时间或事件的异步流程 | 基础攻击的 `WaitInputRelease`、`WaitDelay` |

Gameplay Cue 主要负责音效、特效和动画。ReEcho 当前没有把主要表现迁移到 Cue；只有当表现需要随效果统一添加、复制和移除时再评估引入。

```text
玩家输入
  -> Input Tag
  -> AbilitySpec
  -> Ability 激活检查
  -> CommitAbility
  -> 武器调用或 AbilityTask
  -> GameplayEffect Spec
  -> AttributeSet 结算
  -> 属性委托
  -> Combatant 兼容层 / HUD / 死亡 / 回响记录
```

- GA 决定“能否执行以及执行流程”。
- GE 决定“属性或标签发生什么变化”。
- AttributeSet 决定“变化最终如何结算”。

不要在 GA、武器、敌人和 UI 中各写一份伤害规则。

## 3. 代码地图

以下路径均位于 `Source/ReEcho`。

| 文件 | 阅读重点 |
| --- | --- |
| `Public/AbilitySystem/ReEchoGameplayTags.h` | GAS 标签公共声明 |
| `Private/AbilitySystem/ReEchoGameplayTags.cpp` | Native Gameplay Tag 注册 |
| `Public/AbilitySystem/ReEchoCombatAttributeSet.h` | 战斗属性和访问器 |
| `Private/AbilitySystem/ReEchoCombatAttributeSet.cpp` | 钳制、伤害、格挡、治疗和死亡边界 |
| `Public/AbilitySystem/ReEchoGameplayEffects.h` | 原生 GE 类型与应用入口 |
| `Private/AbilitySystem/ReEchoGameplayEffects.cpp` | 初始化、伤害、治疗和冷却 Spec |
| `Public/AbilitySystem/ReEchoPlayerAbilities.h` | 玩家 GA 类型 |
| `Private/AbilitySystem/ReEchoPlayerAbilities.cpp` | Commit、AbilityTask、武器调用和结束路径 |
| `Private/Combat/ReEchoCombatantComponent.cpp` | GAS 属性到旧接口的适配 |
| `Private/Player/ReEchoPlayerPawn.cpp` | ASC 初始化、授予技能和输入标签分发 |
| `Private/Tests/ReEchoGASAutomationTests.cpp` | GAS 测试环境和项目契约 |

## 4. 运行时生命周期

玩家和敌人在构造函数中创建 ASC 与 AttributeSet。Actor 进入世界后必须调用：

```cpp
AbilitySystemComponent->InitAbilityActorInfo(OwnerActor, AvatarActor);
```

当前单机实现中，玩家的 Owner 和 Avatar 都是 Pawn。ActorInfo 初始化后，才绑定属性委托、应用初始化 GE、授予技能。

属性权威关系：

```text
BuildSnapshot（跨遭遇持久数据）
        |
        v
初始化 GameplayEffect
        |
        v
AttributeSet（当前遭遇运行时权威）
        |
        v
CombatantComponent 镜像 / HUD
```

不要同时直接写 BuildSnapshot、Combatant 字段和 AttributeSet，否则容易产生双重扣血、UI 延迟或跨遭遇污染。

技能通过 `GiveAbility` 授予，输入标签保存在 AbilitySpec 的 Source Tags 中。按键时玩家匹配标签并请求激活，释放时通知 ASC。输入层只发送标签，不直接调用技能实现。

## 5. 新增一个属性

以 `CriticalChance` 为例：

1. 在 `ReEchoGameplayTags.h/.cpp` 注册 `Data.CriticalChance`。
2. 在 `UReEchoCombatAttributeSet` 声明属性和访问器。
3. 在 `PreAttributeChange` 或 `PostGameplayEffectExecute` 中钳制合法范围。
4. 给初始化 GE 添加 Modifier，通过 `Data.CriticalChance` 设置 SetByCaller 值。
5. 仅当旧 UI 或旧调用方需要时，才给 `UReEchoCombatantComponent` 添加只读镜像与委托。
6. 测试初始化值、上下限及连续应用效果。

元数据表中的 Min/Max 不能代替运行时钳制。

## 6. 新增一个 Gameplay Effect

1. 选择持续策略：`Instant`、`HasDuration` 或 `Infinite`。
2. 添加指向 AttributeSet 属性的 Modifier。
3. 动态数值使用已注册的 SetByCaller Tag。
4. 创建 Effect Spec，不修改 GE 类默认对象。
5. 通过 ASC 应用到目标。
6. 最终结算与钳制留给 AttributeSet。

状态效果通过 GE 授予 `State.*` 标签，技能用激活阻挡标签声明限制，不要散落 `bIsStunned` 一类布尔判断。

UE 5.8 中，原生 GE 若添加 `UTargetTagsGameplayEffectComponent` 等 Effect Component，应使用构造函数的 `FObjectInitializer` 创建具名默认子对象，不能在类默认对象构造中临时 `NewObject` 或运行时拼接组件。

## 7. 新增一个 Gameplay Ability

以闪避技能为例：

1. 注册 `Ability.Movement.Dodge`、`Input.Movement.Dodge`、`Cooldown.Movement.Dodge`。
2. 配置技能标签、阻挡状态标签和冷却 GE。
3. 在任何不可逆行为之前调用 `CommitAbility`。
4. Commit 失败立即 `EndAbility`，不得生成 Actor、造成伤害或写回响记录。
5. 瞬时技能执行后结束；持续技能用 AbilityTask 等待事件，并保证成功、取消和销毁路径都结束。
6. 授予技能时把输入标签写入 AbilitySpec。
7. 测试死亡、菜单、眩晕、冷却、输入释放和 Commit 失败。

推荐结构：

```cpp
if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
    return;
}

// 成功后执行不可逆行为。
EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
```

## 8. ReEcho 项目红线

1. BuildSnapshot 是跨遭遇持久构筑来源，AttributeSet 是当前遭遇运行时权威。
2. `UReEchoCombatantComponent` 是兼容门面，不是第二套属性系统。
3. 伤害、治疗、初始化和冷却走 GameplayEffect，不直接改属性字段。
4. 状态优先使用 Gameplay Tag，不增加重复布尔状态。
5. GA 在不可逆行为前 Commit，并在所有路径 End。
6. 主动操作只在成功执行后记录；自动基础攻击不记录。
7. Echo 仍有兼容路径。修改共享武器逻辑时分别验证玩家 GAS 和 Echo 回放。
8. 当前 ASC 留在 Pawn；只有明确的重生持久化或多人需求才迁移到 PlayerState。
9. 不在运行时修改 GE 资产或类默认对象；动态值写入 Effect Spec。
10. UI 只观察属性和状态，不承担战斗结算。

## 9. 常见问题排查

### 技能无法激活

依次检查 ASC 是否初始化、技能是否授予、Spec 是否有正确 Input Tag、是否存在 `State.Dead`/`State.Menu`/`State.Stunned`、冷却标签是否仍存在、Commit 是否失败，以及上次激活是否遗漏 `EndAbility`。

### 效果应用后属性不变

检查 Spec、SetByCaller 标签、目标 ASC、Modifier 指向的属性，以及 AttributeSet 是否钳制或抵消了变化。若使用 `IncomingDamage` 或 `IncomingHealing` 元属性，继续查看 `PostGameplayEffectExecute`。

### 测试中的 ASC 无效

ASC 依赖有效 Actor 生命周期和 ActorInfo。测试应创建临时 `UWorld` 和真实 Actor、注册 WorldContext、初始化 ASC，并在结束时清理。不要用裸 `NewObject<UAbilitySystemComponent>` 模拟完整流程。

### UI 与战斗数值不一致

检查是否仍有直接写 Combatant 字段的旧路径。运行时变化应进入 GE/AttributeSet，再通过属性委托刷新兼容层和 HUD。

## 10. 调试与验证

项目 GM 指令可快速制造状态：

- `GMStatus`：查看当前状态。
- `GMHeal <Value>`：测试治疗及上限。
- `GMKillAll`：快速推进遭遇。
- `GMWeather <Type>`：切换天气。

完整指令见 `docs/GM_COMMANDS.md`。

修改 GAS 后执行：

```powershell
cmd.exe /c scripts\ue\Build-Editor.cmd
cmd.exe /c scripts\ue\Run-Automation.cmd ReEcho.
python scripts\validate_project.py
git diff --check
```

构建和自动化前关闭 Unreal Editor。至少手工验证：进入遭遇、攻击、受伤、治疗、死亡、打开/关闭菜单、重新开始遭遇。

## 11. 适合新人的第一个任务

1. 为已有属性增加只读 HUD 展示，并通过 Attribute 委托刷新。
2. 新增一个有边界钳制的非伤害属性及测试。
3. 新增一个短时状态 GE，并用标签阻挡现有技能。
4. 为现有 GA 增加 Commit 失败和冷却标签测试。

第一次修改不要同时新增 Attribute、GA、复杂 AbilityTask、网络预测和 Gameplay Cue。先沿现有路径完成一个闭环。

## 12. 官方资料

- [GAS 总览](https://dev.epicgames.com/documentation/unreal-engine/gameplay-ability-system-for-unreal-engine)
- [ASC 与 Gameplay Attributes](https://dev.epicgames.com/documentation/unreal-engine/gameplay-ability-system-component-and-gameplay-attributes-in-unreal-engine)
- [Attributes 与 Attribute Sets](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-attributes-and-attribute-sets-for-the-gameplay-ability-system-in-unreal-engine)
- [Gameplay Effects](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-effects-for-the-gameplay-ability-system-in-unreal-engine)
- [Gameplay Abilities](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-abilities-in-unreal-engine)
- [Gameplay Tags](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-tags-in-unreal-engine)
- [Ability Tasks](https://dev.epicgames.com/documentation/unreal-engine/gameplay-ability-tasks-in-unreal-engine)