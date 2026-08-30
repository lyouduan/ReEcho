# Plan 142 - 程序 - 回响连接线与出生法阵特效

## 协调

- Planner 负责人：当前对话程序 Planner（Codex）。
- Executor 负责人：当前对话程序 Executor（Codex）。
- Plan 编写方（AI 侧）：`ReEcho teammate-side AI`。
- 实现编写方（AI 侧）：`ReEcho teammate-side AI`。
- 任务状态：`Executing`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@73030025dc262fe82fc4f530c5b1f9154c946a7d`。
- 本地实现方式：独立 worktree `C:\tmp\ReEcho-plan142-echo-connection-vfx`，分支 `codex/plan142-echo-connection-vfx`。
- 依赖 / 阻塞：卡牌 `G_2_30` 已通过 `FReEchoCardRuleSnapshot::bConnectionLineDamage` 驱动正式穿线伤害；外部交付资产为 `F:\Content\VFX\Echo\Particle\NS_Echo_Chain.uasset`。出生法阵复用仓库已有 Goat 地面法阵结构与 Echo 材质/纹理依赖，不修改共享 Boss 原资源。
- Writes:
  - `plans/142-programmer-echo-connection-line-vfx.md`
  - `Content/VFX/Echo/Particle/NS_Echo_Chain.uasset`
  - `Content/VFX/Echo/Particle/NS_Echo_Born.uasset`
  - `Content/VFX/Echo/MI/BaseVFX003_Inst25.uasset`
  - `Content/01_Textures/02_Turbulence/Tur_C080.uasset`
  - `Content/01_Textures/02_Turbulence/Tur_C090.uasset`
  - `Source/ReEcho/Public/Presentation/VFX/ReEchoCombatVfxCatalog.h`
  - `Source/ReEcho/Private/Presentation/VFX/ReEchoCombatVfxCatalog.cpp`
  - `Source/ReEcho/Public/Presentation/VFX/ReEchoCombatVfxComponent.h`
  - `Source/ReEcho/Private/Presentation/VFX/ReEchoCombatVfxComponent.cpp`
  - `Source/ReEcho/Public/Graybox/ReEchoEchoActor.h`
  - `Source/ReEcho/Private/Graybox/ReEchoEchoActor.cpp`
  - `Source/ReEcho/Public/ReEchoGameMode.h`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoCombatVfxTests.cpp`
  - `Source/ReEcho/Private/Tests/ReEchoGameModeTests.cpp`（仅在现有测试夹具可稳定覆盖同步入口时）
  - `scripts/ue/audit_plan142_echo_chain_vfx.py`
  - `scripts/ue/create_plan142_echo_born_vfx.py`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - FullRebuild 刷新的 `Binaries/Win64/ReEchoEditor.prebuilt.json` 及其准确允许列表产物
- Stable Reads:
  - `Content/Data/cards.csv`
  - `Content/Data/card_effects.csv`
  - `Source/ReEchoCards/Public/Cards/ReEchoCardTypes.h`
  - `Source/ReEchoCards/Private/Cards/ReEchoCardRuntime.cpp`
  - `Source/ReEcho/Public/Graybox/ReEchoEchoActor.h`
  - `Source/ReEcho/Private/Graybox/ReEchoEchoActor.cpp`
  - `Source/ReEcho/Public/Player/ReEchoPlayerPawn.h`
  - `Source/ReEcho/Private/Player/ReEchoPlayerPawn.cpp`
  - `Source/ReEcho/Public/Combat/ReEchoCombatTarget.h`
  - `Source/ReEcho/Private/ReEchoGameMode.cpp`
  - 已存在的 `/Game/VFX/Element/Elctricity/MI/BaseVFX003_Inst1`、`Inst27`、`Inst28` 与 `/Game/Mat/BaseVFX003`
- 影响模式：`SharedContract`；新增 Cards 规则到 VFX adapter 的持续连接表现，并新增 Echo 初始化成功后的单次出生表现，但不改变 Cards、Combat、Recording 或 Echo 生命周期公共状态所有权。
- 兼容承诺 / 下游操作：`G_2_30` 的伤害公式、穿越判定、敌人命中频率、玩家/回响位置和多回响循环保持现状；缺失或失效 VFX 只能少表现，不能阻止卡牌伤害。
- 明确排除：不修改 XLSX/CSV、卡牌数值、伤害公式、穿线算法、Echo 数量、回放、武器、导电特效或外部 Echo Water/Grass/LevelSequence 资产；不导入 `NS_Echo_Chain` 实际引用闭包之外的资源；出生法阵不增加无敌、移动/攻击门禁、角色淡入或录制延迟，不修改 `NS_Goat_Skill03_Alarming` 及其共享材质。

## 锁定目标

持有 `G_2_30「连接，连接！」` 时，角色本体与每个存活回响之间各维持一条 `NS_Echo_Chain` 链接；双方移动时端点连续跟随。双回响必须显示两条并与现有两条玩法伤害线一致。卡牌未生效、任一端死亡/失效、回响移除、遭遇结束或宿主 EndPlay 时，准确清理对应链接且无残留。资源缺失时玩法继续。

每个回响仅在外观、录制、武器和战斗属性全部初始化成功后，以其最终表现脚底中心播放一次独立 `NS_Echo_Born`。法阵保持世界尺寸，生成后留在该出生点而不附着或跟随回响；双回响各播放一次。第一关转第二关的专用镜头流程在后台预置并隐藏回响，镜头定位后在同一次调用中启动法阵并显示回响及武器，保持 `0.4` 秒后继续镜头拉远，法阵本身仍播放到原生命周期结束；资源缺失则立即显示回响并继续镜头。Development `GMEchoBorn` 只在当前存活回响位置重播表现，不生成回响或修改玩法状态。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`、`AREA-Cards`、`AREA-Presentation`、`AREA-Tests`；`MOD-ReEchoCards` 仅稳定读取，不改变其契约。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md` 的卡牌持续链接生命周期和资产契约；维护 `shared/CODEBASE_MAP/modules/MOD-ReEcho.md` 的 GameMode Cards→Presentation 接线；两者已加入 Writes。审阅 `MOD-ReEchoCards.md`，规则快照所有权不变则不制造正文 diff。
- 设计意图：Cards 继续只声明 `bConnectionLineDamage`，GameMode 在既有 Encounter tick 读取同一规则和存活 Echo 集合，VFX Component 只维护 Niagara 组件与端点；不得把表现 Actor 或 Niagara 状态写回 Cards/Combat。
- 出生表现设计：Echo Actor 在 `InitializeEcho` 成功路径调用 VFX adapter；`GroundRoot` 是落点高度和位置权威，Niagara 是一次性世界特效，不成为 Echo 初始化、回放、攻击或存活状态的权威。GM 入口复用同一播放函数。
- 权威状态与依赖：现有 Player/Echo Actor XY 仍是穿线伤害线段权威。视觉端点使用相同 XY，并只从双方 `HurtVfxRoot` 取得各自 Z 高度，避免表现偏移改变玩法几何。每个 Echo 的 Link 由 Player 的 VFX Component 以 weak Echo key 独立拥有。
- 决策记录：
  1. 外部包字符串闭包显示 Chain 直接引用 Echo `BaseVFX003_Inst25` 和仓库已有 Electricity `Inst1/27/28`；`Inst25` 再引用缺失 `Tur_C080/Tur_C090` 及仓库已有 `Hor_C062/Mas_C010/BaseVFX003`。仅导入 Chain、Inst25、Tur_C080、Tur_C090。
  2. 不把 Niagara Component 附着到 Player 或 Echo；以世界原点、identity rotation、World Space 参数更新两个端点，避免双重 Local/World Transform。资源导入后必须用 UE 读回确认真实参数类型和 End 语义；若不是可无损适配的双端点系统则停止接入并报告。
  3. 多回响一对一映射，新增/保留/移除均按存活集合差量同步；不每帧重新 Spawn。
  4. GameMode 只传规则开关与 Echo Actor 集合，不直接持有 Niagara Component；VFX 缺失 fail-open。
  5. `NS_Echo_Born` 从 `NS_Goat_Skill03_Alarming` 复制为独立资产，再只在副本内调整一次性时长、地面朝向和 Echo 配色可用参数；不得修改 Boss 原 System 或共享材质。无法安全改色时保留独立候选并交由 PIE/美术调色，不扩大共享资产范围。
- 相关文档同步范围：更新 `MOD-ReEchoVFX.md` 与 `MOD-ReEcho.md`；审阅 `ARCHITECTURE.md`、`README.md`、`MOD-ReEchoCards.md`，模块拓扑、稳定标识和 Cards 规则所有权不变时记录无需修改。
- 关闭前逐项填写审阅结果：待实现、自动化及 PIE 验收后补充。

## 锁定验收

- [ ] 精确资源闭包可加载：Chain、Inst25、Tur_C080、Tur_C090；未导入无关 Echo 资产，仓库已有依赖未被覆盖。
- [ ] `NS_Echo_Chain` 的启用 Emitter 坐标空间、Renderer、材质和端点参数由 UE 回读锁定；运行时写入顺序与资产真实语义一致。
- [ ] 单回响一条、双回响两条；移动时视觉线段端点跟随角色和对应回响，并与玩法 XY 线段一致。
- [ ] 卡牌关闭、对象死亡/失效、Echo 集合变化、遭遇结束和 EndPlay 均准确清理；资源失败不影响现有物理伤害。
- [ ] Development FullRebuild、VFX/GameMode 聚焦自动化、`python scripts/validate_project.py`、预构建检查和 `git diff --check` 通过。
- [ ] 用户在 PIE 验收端点高度、遮挡、亮度、宽度、移动稳定性及双回响画面。
- [ ] `NS_Echo_Born` 独立加载且不覆盖 Goat 原资源；法阵以 `GroundRoot` 世界位置生成、平行贴地、一次性结束并留在出生点。
- [ ] Echo 初始化失败不播放法阵；单/双 Echo 成功初始化分别播放一/两次；资源缺失 fail-open；`GMEchoBorn` 只重播表现。
- [ ] 用户在 PIE 验收出生法阵颜色、大小、贴地、透明排序、亮度、结束残留和双回响画面。

## Step 0 门禁

- 基线分支/提交：`origin/main@73030025dc262fe82fc4f530c5b1f9154c946a7d`。
- 引擎/构建可用性：UE 5.8 Windows；Editor/命令使用 Git-common-dir Unreal 锁。Plan140 导电候选保留在另一独立 worktree，本任务不得读取其未提交实现作为基线。
- 现有聚焦测试结果：静态确认 `G_2_30` 已编译为 `bConnectionLineDamage`；GameMode 对每个存活 Echo 执行独立穿线判定和 `0.5 * (PhysicalAttack + ElementalAttack)` 物理伤害；当前没有对应持续链接 VFX。
- 共享契约 / 难合并资源风险：`ReEchoGameMode.cpp` 是共享高碰撞文件，但本任务已从包含 Plan135 最新修改的远端基线创建；Niagara/MI/纹理为不可文本合并资产，发布前必须审计远端同路径变化。
- 基线损坏时的停止条件：外部资源还依赖未交付且仓库缺失的路径；资产端点不是可运行时更新的双端点契约；必须修改玩法伤害才能对齐表现；或远端并行修改相同 Chain/Inst25/纹理。

## 实现提纲

1. 发布本 Plan 后，把精确外部闭包复制到相同 `/Game/` 路径，并通过 UE 只读审计确认加载、依赖、Emitter/Renderer 和 User Parameter。
2. 在 VFX Catalog 注册卡牌链接语义；在 Player VFX Component 中维护 `Echo -> NiagaraComponent` 映射，按规则与存活集合差量同步，并每帧更新双方世界端点。
3. 在 GameMode 既有 Cards Encounter tick 中调用同步入口，复用其同一份 RuleSnapshot 和 Echo 集合；所有停止路径调用清理。
4. 扩充资产契约和生命周期自动化，更新模块文档与 Plan 执行记录。
5. 复制 Goat 地面法阵为独立 Echo Born System，审计依赖与 Renderer 后，在 Catalog 注册 `EchoBorn`；Echo 初始化成功路径和 Development GM 入口均调用 VFX adapter 的同一世界落点播放函数。
6. 冻结候选后执行完整门禁，最后请求 PIE 视觉验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 资源闭包 | UE Plan142 审计 + Asset Registry/加载回读 | 精确四个新资源加载；全部硬依赖存在；参数类型、空间和 Renderer 材质明确 |
| C++ | `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新精选 7 模块预构建包 |
| 自动化 | `scripts\ue\Run-Automation.cmd -Filter ReEcho.Presentation.VFX.Catalog` 及具名 GameMode/VFX 链接测试 | Catalog、端点更新、多 Echo 差量生命周期、清理和 fail-open 通过 |
| 静态 | `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`git diff --check` | 数据无漂移、预构建指纹和差异格式通过 |
| 人工 PIE | 授予 `G_2_30`，单/双 Echo 移动并让敌人穿线 | 视觉与伤害线一致、无抖动残留、亮度宽度可接受 |
| 出生法阵资源 | UE 加载/依赖/Emitter/Renderer/朝向审计 | 独立 `NS_Echo_Born` 可加载，Goat 原资源未修改，全部硬依赖存在 |
| 出生法阵自动化 | VFX Catalog + Echo 初始化/GM 聚焦测试 | 成功初始化播放、失败不播放、世界 GroundRoot 落点、双 Echo 独立和 GM 纯表现契约通过 |
| 出生法阵 PIE | 单/双 Echo 自然出生与 `GMEchoBorn` 重播 | 贴地平行、留在出生点、一次性结束、颜色/尺寸/排序可接受，无玩法时序变化 |

## 执行记录

### 变化

- 从 Goat Skill03 地面法阵复制独立 `/Game/VFX/Echo/Particle/NS_Echo_Born`，保持 Boss 原 System 与共享材质不变；副本继续使用显式世界 Up 地面朝向。
- Catalog 新增 `EchoBorn`，VFX adapter 在 GroundRoot 世界位置播放并以 0.8 秒有限计时销毁；特效生成后不附着回响移动。
- PIE 反馈确认法阵仍偏大，`EchoBorn` 以 PreserveWorldSize 策略把统一倍率从 `0.5` 调整为 `0.25`，即相对上一版再缩小 50%，位置、朝向与生命周期不变。
- 按用户参考图确认的冰蓝配色复制 Inst15/21/22/23 为 4 个独立 `MI_Echo_Born_*`，并仅重绑 `NS_Echo_Born` 的 Sprite/Ribbon/Mesh Renderer。PIE 证明仅调材质会被 Niagara 内部紫色粒子曲线重新染紫，因此副本内嵌 Color Curve 与颜色 Rapid Iteration 常量按原强度统一映射；为增强短时可读性，粒子采用 HDR 冰蓝比例 `(0.75, 1.17, 1.50)`，独立材质使用 `1.35` 中性 HDR 增亮；Goat 原 System、曲线与材质实例不变。
- 侧视 PIE 发现粒子均从 `Z=0` 生成，但 `0817_01` Mesh 本身高度约 `11.49`，且 Fountain004/006 使用 `Mesh Scale Z=40`，导致法阵形成高于粒子平面的厚层；仅在 EchoBorn 副本把两个 Mesh Scale Z 压平为 `0.1`，XY 仍为 `25`，不改共享 Mesh。
- 压平 Mesh 后 PIE 显示上升粒子视觉高度不足；审计确认此前通用地面朝向工具把所有 Sprite Renderer 一并设为 OwnerUp。EchoBorn 改为从 Goat 源 System 按 Emitter/Renderer 索引恢复 Sprite Facing、Alignment 及绑定，仅 Mesh 法阵承担贴地平面，烟雾/光粒恢复源特效的相机朝向与视觉高度。
- Echo 初始化成功后登记一次待播放状态；首次 `AdvanceEcho` 先应用录制位置，再触发出生法阵，初始化失败不播放。`GMEchoBorn` 只对当前存活回响复用同一表现入口。
- 第一关转第二关的预加载路径改为延迟显形：先把所有回响推进到录制起点并隐藏本体/武器，镜头锁定并保持静止时播放各自法阵，使用暂停 Tick 累计完整 `0.8` 秒后统一显形并启动后续镜头；法阵生成失败、镜头流程降级、重置或正式激活均 fail-open 恢复可见性。
- 出生 Niagara 只读取 Echo 最终阴影组件 `GroundShadow` 的世界中心作为生成位置，不附着任何 Echo 组件；它以未激活的独立世界组件创建，先设置暂停 Tick、排序和世界变换，再显式激活，避免继承隐藏 Echo 的可见性并保证暂停期间完成首帧初始化。
- 新增 Development `GMEchoSummon`，对全部存活回响在当前位置复播“隐藏本体/武器 → 启动法阵 → 0.4 秒后显形”流程；既有 `GMEchoBorn` 保持只播法阵，便于分别检查资产和时序。
- 第一关转第二关采用串行起播表现：镜头先瞬时定位隐藏 Echo 并保持静止，启动一次性法阵，0.4 秒后显示 Echo/武器并开始 1 秒镜头拉远及后续移向玩家；法阵继续播放到原生命周期结束，特效或镜头失败时 fail-open 显形并继续。
- 出生法阵在最终 CG 画面仍遮挡世界时先激活，随后才关闭过渡界面，保证回到游戏画面的第一帧已经处于法阵播放中，而不是先露出空场再启动特效。
- CG 遮罩下完成定位后解除全局 Pause，但保持 Prepared Encounter、输入、敌人模拟、Director、Recording 与 Echo 回放门禁；正式显形在法阵启动满 0.4 秒后执行，不再等待 Niagara 的完整生命周期，法阵生成失败则立即 fail-open 显形。
- 法阵尺寸不再由固定 `0.25` 决定正常路径：Echo 复用当前 Flipbook 空间宽度，目标世界直径为角色宽度的 `1.2` 倍，再按 Niagara authored XY Bounds 换算统一缩放；Bounds 无效时才回退 Catalog `0.25`。全部启用 Sprite Renderer 绑定 `User.GroundNormal=(0,0,1)`，不再恢复 FaceCamera。
- 2026-08-30 尺寸复调：正常路径的目标世界直径从 Echo 当前表现宽度的 `0.8` 倍缩小到 `0.4` 倍，即上一版表现的 50%；位置、朝向、生命周期、1→2 关专用触发条件以及 Bounds 无效时的 Catalog `0.25` 回退均保持不变。
- 2026-08-30 Mesh 平面修复：`0817_01` 的真实可见平面是本地 YZ，旧作者化误把 `Fountain004` 唯一的 `InitializeParticle.Mesh Scale Z` 从 `40` 压到 `0.1`，使其可见图案被单独压扁，而 Sprite Fountain002/003/005 不受影响。现恢复可见 `Y=25、Z=40`，仅把平面法线/厚度轴 `X` 压到 `0.1`；统一 Component 尺寸倍率继续独立生效。
- 2026-08-30 统一缩放修复：运行时目标直径换算改为只读取 Niagara 本地 YZ 可见平面的 `Max(Y,Z)`，不再把本地 X 法线/厚度纳入直径。Fountain002/003/004/005 继续共同消费同一个均匀 Component WorldScale；自动化锁定 X Bounds 任意变化都不影响最终尺寸。
- 2026-08-30 Fountain004 尺寸恢复：按人工验收要求取消该 Emitter 的任何局部压缩，唯一 `InitializeParticle.Mesh Scale` 恢复源资源完整 `(25,25,40)`；Fountain004 与其他层只共同消费运行时统一 Component WorldScale。
- 2026-08-30 整体尺寸复调：统一 Component 目标直径从 Echo 当前表现宽度的 `0.4` 倍缩小为 `0.2` 倍，即当前表现的 50%；Fountain004 继续保持原始 `(25,25,40)`，全部 Emitter 同比缩小。
- 镜头旋转复验表明 `GroundShadow` 的 XY 包含随镜头变化的 2D 脚点补偿，不能作为生成后固定的世界法阵中心；法阵改用 Echo Actor 的稳定世界 XY，并只读取 `GroundShadow` 的地面 Z。Sprite 除 `User.GroundNormal=(0,0,1)` 外再绑定 `CustomAlignment` 的 `User.GroundTangent=(1,0,0)`，同时锁定平面法线和面内方向，避免绕世界 Up 继续追随相机旋转。
- PIE 首次加载曾因资产脚本 `RequestCompile` 后立即保存退出，把 Echo Born 的待处理 Niagara 编译遗留给运行时，产生约 23 秒主线程等待。新增统一 `CompileNiagaraSystemAndWait` authoring seam，两个 Echo Born 脚本均在保存前等待 CPU/GPU 编译完成并确认无 outstanding request；修复后 commandlet 内该 System 编译分别为 0.18 秒和 0.14 秒，编译结果随资产保存。
- 精确导入 `NS_Echo_Chain`、`BaseVFX003_Inst25`、`Tur_C080`、`Tur_C090`；未复制外部目录中的 Water/Grass/LevelSequence 或其他无关材质、纹理。
- Catalog 新增 `EchoConnectionLine`，Player VFX Component 按存活 Echo 集合差量维护持续 Niagara，并从双方当前 Flipbook 渲染 Bounds 中心更新世界端点。
- 持续链条逐帧按两端的 effect-local 坐标更新有限 System Fixed Bounds（含 200cm Ribbon 安全边距），避免双方距离拉长后被 Niagara 资产 Bounds 错误剔除。
- 连接线唯一使用的 `DamageSource::Path` 在实际造成正伤害时复用 `EnemyHurt`，改为在命中世界位置生成一次性受击特效；致死连接线命中也保留该表现，避免随目标死亡清理而立即消失。
- Boss 作为受击目标时，`EnemyHurt` 的世界位置从作者 `HurtVfxRoot` 向当前 Flipbook 渲染中心抬高一半，避免特效下半部分埋入地面；普通怪物挂点和 Boss 攻击命中位置不变。
- GameMode 在既有卡牌固定步投影 `bConnectionLineDamage` 与存活 Echo；普通清场和 Boss 阶段退休 Echo 均显式清理，组件 EndPlay 继续通过 `StopAllEffects` 兜底。
- 自动化补充 Chain 路径、资产加载及两个 LWC Niagara Position 参数检查；模块文档同步运行时所有权和生命周期。
- 根据 PIE 反馈，将持续链条端点刷新从仅固定步补充到 VFX Component 每帧 Tick；固定步继续只负责卡牌开关与 Echo 集合差量，避免角色最终 Transform 晚于固定步造成视觉停留旧位置。
- 第二轮 PIE 反馈确认视觉仍未消费动态参数：链条端点改为双方当前 Flipbook 渲染 Bounds 中心，并在端点实际移动时重初始化该 Chain Niagara，使仅在粒子初始化阶段采样 Beam 参数的 Ribbon 取得新位置；导电保持本任务外，暂不修改。
- 链条持续时间改由卡牌和双方存活状态持有；Niagara 自身完成后只要本体与对应回响仍存活便重新激活，死亡、回响消失或卡牌关闭才销毁。
- 第三轮 PIE 的间歇消失由移动时反复 `ReinitializeSystem` 导致；移除端点缓存和重初始化，改为 Chain Niagara 依赖 VFX adapter Tick，同一持续实例在端点写入后再模拟。
- 第四轮 PIE 确认仅调整 Tick 顺序仍不足：通过 UE 编辑接口为 `NS_Echo_Chain` 启用 Emitter 的 Particle Update 栈补齐 `Update Beam`，让已存在 Ribbon 每帧消费变化后的 Beam Start/End。

### 证据

- `create_plan142_echo_born_vfx.py` 经 UE Commandlet 完成复制、世界 Up 朝向保存和依赖审计；日志 `PLAN142_ECHO_BORN_COMPLETE`，副本依赖现有 Goat Mesh `0817_01` 与 Inst15/21/22/23，原资源未修改。
- 最终组合候选 Development FullRebuild：101 actions 全部通过，精选 7 模块预构建包刷新并由 `prebuilt_editor.py check` 验证，源码指纹 `6924612321f5`。
- `ReEcho.Presentation.VFX.Catalog` 1 项通过，包含 `NS_Echo_Born` 加载、独立路径、世界尺寸和 0.8 秒生命周期契约；`ReEcho.Presentation.EchoAppearance` 2 项通过。
- 最终 `validate_project.py`、LFS checkout/fsck 与 `git diff --check` 通过。
- UE Plan142 资产审计：四个新资源全部加载；Chain 直接引用新 Inst25 和仓库已有 Electricity Inst1/27/28，Inst25 的新增依赖只有 Tur_C080/Tur_C090，其余依赖均已存在。
- `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild`：通过，98 actions；精选 7 模块预构建包已刷新。
- `scripts\ue\Run-Automation.cmd -Filter ReEcho.Presentation.VFX.Catalog`：找到 1 项并通过；日志 Result=Success。日志同时报告既有 Echo Water Aura 的两个缺失依赖，不属于本次 Chain 闭包。
- `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`git diff --check`：通过；首次沙箱运行仅因 Content 临时目录权限被拒，授权后的同一校验完整通过。

### 剩余风险

- `NS_Echo_Born` 已使用独立中性材质、冰蓝白粒子颜色曲线及 `0.5` 倍尺寸；实际饱和度、透明排序、0.8 秒截断和双回响叠加亮度仍需 PIE 人工验收后再调，不修改共享资源。
- `-nullrhi` 自动化不能证明 Chain 的屏幕空间宽度、亮度、Ribbon 朝向或移动稳定性；仍需 PIE 验收单/双 Echo。
- 当前 Chain 按其 Beam 模板契约使用绝对 Start + 相对 End；资产实机若采用不同渲染解释，应只在 VFX adapter 调整，不改连接线玩法几何。

### 人工验收结果/请求

`PendingBeforeClose`：实现后请求用户在 PIE 验收持续链接的视觉质量和玩法线一致性。

### 架构文档审阅结果

- 已更新 `MOD-ReEchoVFX.md` 与 `MOD-ReEcho.md`。
- `ARCHITECTURE.md`、根 `README.md` 与 `MOD-ReEchoCards.md` 的模块拓扑、稳定 ID 和 Cards 规则所有权未变化，无需正文修改。
