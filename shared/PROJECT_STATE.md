# ReEcho project state

Last updated: 2026-07-29

## Playable state

- The repository contains a source-first UE 5.8 C++ graybox prototype and design JSON registry.
- `/Game/Level00` is the editor startup and packaged-game default map; `AReEchoGameMode` generates the runtime arena and gameplay actors inside it.
- The current loop includes GAS-routed player attacks/weapon selection with packaged DeveloperSettings weapon definitions, swept sphere projectiles, four enemy archetypes, world health bars, damage VFX and hit reactions, deterministic recording, echo playback and automatic six-encounter advancement.
- Player, echo, six normal-enemy variants and the final boss use packaged 2D Billboard textures with soft ground shadows; player and enemies use center-aligned capsule hit volumes. ClockKeeper is merchant art and is no longer spawned or configured as an enemy. Collision debug drawing defaults off and can be toggled with `ReEcho.DebugCollision 0|1`.
- Encounter populations now begin with 10 grunts plus per-encounter growth (configured in DefaultGame.ini); spawn positions are deterministic-random within a configurable 450–850 unit ring around the player, clamped inside the scene boundary, and the final Boss encounter includes peripheral adds.
- Runtime weather presentation supports configurable rain and fog encounters. Rain uses animated screen-space streaks; fog uses layered drifting translucent bands. Both are visual-only and input-transparent.
- Inventory and shop menus use the supplied 16:9 illustrated backgrounds. B opens run-local owned items, M opens four Time Shard purchases, and successful purchases are duplicate-guarded and immediately update the shared build snapshot.
- Four NewCast player appearances are available through `DefaultCharacterId`: J_HEART/J_SPADE/J_CLOVER/J_DIAMOND.
- Player, echo and all enemy archetypes now use Billboard-only character rendering; legacy hidden sphere/cube/plane/cone actor placeholders have been removed.
- Player, echo and 2D enemies add sprite-local idle, attack and hit motion to their static textures; enemy death briefly shrinks/falls without moving collision authority.
- Esc opens a packaged-build pause menu with resume, full restart and quit; the death screen supports restart and quit.
- Defeating the encounter-6 Boss opens a terminal victory settlement with 6/6 completion, time shards, trait count, restart and quit; final-Boss timeout is a failure and cannot show victory.
- UE 5.8 Editor Development and Windows Shipping builds succeed. Four ReEcho automation tests pass, and the latest clean-cooked Windows package includes all five MushroomGirl animation textures and passes a 12-second launch smoke test.
- Human PIE play-feel acceptance is still required; the project must not be presented as a finished vertical slice.

## Current progress

- Milestone A combat skeleton is implemented as a runtime-generated 2.5D prototype with 2D actors over a 3D arena.
- Player input activates dedicated GAS abilities; weapons still own compatible damage/cooldown execution, and projectiles use continuous capsule-path hit tests.
- Enemies include grunt, shield, bomber and boss behaviors plus stagger, knockback, shake feedback, fixed capsule hit volumes and camera-facing health bars.
- Static 2D character textures now have a programmatic animation layer for idle bob, attack lunge/pulse, hit squash/shake and enemy death; Paper2D sequence assets can replace this layer later.
- MushroomGirl has three generated idle frames, two attack frames and a transparent 5-column source Sprite Sheet. The five frames are imported as cooked Texture2D assets and the player Billboard switches them at runtime: 4 FPS idle loop and attack-state playback.
- The player sprite mirrors left/right from the mouse position in camera screen space while collision and attack targeting remain unchanged.
- Recording/playback and six-encounter run seams are connected end to end.
- AI workflow, code routing, installed-engine scripts, UE MCP and lazy RenderDoc MCP guidance are present.

## Milestones

| Milestone | Scope | Status | Exit gate |
|---|---|---|---|
| A - Combat skeleton | Player, projectiles, four enemies, encounter clock, recording and echo | Implemented and packaged; tuning remains | Broader combat determinism and play-feel tuning |
| B - Planning loop | Preview, recorded path, route preview, setup beat, collaboration telemetry | Not started | Five-player direction check |
| C - Build/run | Stats, four characters/weapons, cards, shops, currency, six encounters | GAS input and trait loop connected; content/data migration partial | Data importer + full content run |
| D - Elements/keystones | Reactions, paths, multi-echo, anchor, Boss | Partial seams only | Vertical slice acceptance |
| E - Validation | Fixed-seed regressions, tuning, A/B test | Four automation tests + Windows Shipping smoke test | Go/No-Go report |

## Collaboration protocol

- Default branch is planner-owned; executors work in isolated branches/worktrees and do not merge.
- Human owns final acceptance and play-feel decisions.
- Shared/merge-hostile UE assets require an ownership row in `PLANNER_EXCHANGE.md`.
- `shared/` is the externalized memory bus. `CODEBASE_MAP.md` is the shortest authoritative code retrieval index.
- Generic workflow improvements go back to the canonical blueprint; ReEcho-specific decisions stay in project rules/state.

## Verified toolchain

- Engine: UE 5.8 installed/release build; the separate source checkout is out of scope.
- Build: `scripts/ue/Build-Editor.cmd` succeeds for ReEchoEditor Win64 Development.
- Automation: `scripts/ue/Run-Automation.cmd -Filter ReEcho` passes GAS structure plus two recording tests.
- Packaging: a clean Windows Shipping Cook includes all four 2D actor textures and the resulting EXE passes a 12-second launch smoke test.
- Static: `scripts/validate_project.py` validates JSON registries and workflow files.
- Style: project `.clang-format`, reflection-macro layout gate and `git diff --check` pass.

## Regression risks and technical debt

- `Content/Data/*.json` is not cooked/runtime data; gameplay still uses provisional C++ values.
- The runtime arena is not a committed test map and has no asset-generation pipeline.
- GAS structure, recording and final-Boss victory gating have automation; projectile damage, pause/restart/quit, round advancement and hit reactions still need deterministic tests.
- Experimental `AllToolsets` can emit unrelated GameFeatureData/Niagara Python commandlet warnings.
- Human PIE checks remain necessary for movement, projectile hit feel, health-bar readability, echo clarity and full packaged-menu interaction.
## 2026-07-28 角色与回响成对更新

- 新设定表按列拆分为 5 组：`J_CAT`、`J_HEART`、`J_SPADE`、`J_CLOVER`、`J_DIAMOND`。
- 上排导入为 `Player_*`，下排导入为 `Echo_*`；回响初始化时读取录制快照 `CharacterId`，自动选择同列形态。
- `J_CAT` 设为默认角色；透明源图保存在 `Content/SourceArt/Characters/NewCast/Sprites/`，运行时纹理保存在 `/Game/ReEcho/Textures/Characters/NewCast/`。
- 验证：17 个 NewCast Texture2D 已保存，ReEchoEditor Win64 Development 编译成功，5 项 `ReEcho.*` 自动化测试全部通过。
## 2026-07-28 场地背景替换

- 用户提供的 1376x768 空场地图已保存为 `Content/SourceArt/Scenes/ArenaBackground.png`，并导入 `/Game/ReEcho/Textures/Scenes/ArenaBackground`。
- `AReEchoGameMode` 构造期硬引用背景纹理，`CreateArena()` 创建无碰撞、无阴影的水平地面美术平面；旧灰盒地板和四面墙保留碰撞但隐藏渲染，场景图不作为 Skybox。
- 导入成功；ReEchoEditor Win64 Development 编译与 DLL 链接成功，5 项 `ReEcho.*` 自动化测试通过；场景构图仍需 PIE 人验。
- 回响与玩家统一使用 350 世界高度、中心对齐 Billboard，以及相同的阴影偏移和缩放。
- 水平地面版本已完成最终 DLL 链接，5 项 ReEcho.* 自动化测试通过；场景图仅作为地面，不作为 Skybox。
- 场地美术升级为 ArenaGround3D：在保持水平地面用法和空旷战斗区的前提下，以烘焙光影、环境遮蔽和前中后景层次模拟 3D 深度；旧 ArenaBackground 资产保留回退。
- ArenaGround3D 现作为固定相机的天空盒式远景板；实际活动由不可见地板和 ±1100 四面碰撞墙限制，敌人最大出生半径 920。
- 远景板现由玩家相机变换计算位置与朝向：位于相机前方 3000 单位、法线正对相机、16:9 留边覆盖，修复场地图仅显示在画面顶部的问题。
- 最终将 StaticMesh 平面替换为 `UBillboardComponent`：直接显示 `ArenaGround3D`，保持贴图方向和宽高比，并随相机朝向；世界高度 4200 用于覆盖不同 PIE 视口。ReEchoEditor 编译链接成功，5 项 `ReEcho.*` 自动化测试通过，最终画面仍需 PIE 人验。
- PIE 人验发现大型 `UBillboardComponent` 背景不显示，已改为相机基向量对齐的无光照 StaticMesh 平面，并使用 `DefaultSpriteMaterial` 直接采样场景纹理；同时修正 UE 5.8 基础立方体资源路径，恢复隐藏地板和四面活动边界。ReEchoEditor 链接成功，5 项自动化测试通过。
- 白色背景来自 `DefaultSpriteMaterial` 仅由 Billboard 渲染代理注入纹理；已生成 `/Game/ReEcho/Materials/M_ArenaBackground` 专用双面 Unlit 材质，将 `ArenaGround3D` 直接连接 Emissive，并由 GameMode 构造期硬引用。ReEchoEditor 编译链接成功，5 项自动化测试通过。
- 玩家 X/Y 可移动至 ±1050；战斗相机以固定偏移跟随，但焦点仅在 ±450 内移动，到地图取景边缘后停止滚动，玩家可继续走向画面角落。远景与怪物均保持世界固定且不绑定相机，因此不会向外扩展，二者相对位置稳定；远景高度保持 8400（原图方案的 2 倍）。ReEchoEditor 编译链接成功，5 项自动化测试通过。
- 画面比例再次收紧：远景高度由 8400 调为 6300；玩家与回响高度由 350 调为 280，普通怪物/小 Boss/Boss 分别调为 204/240/224。对应胶囊、阴影、血条锚点和宽度同步按约 0.8 缩放，玩家重置高度调为 140。ReEchoEditor 编译链接成功，5 项自动化测试通过。
- 玩家、回响和所有怪物 Billboard 使用相机视线深度补偿：以 1000 世界单位为基准，按当前 ViewDepth/1000 调整视觉缩放并限制在 0.35–3.0，使镜头移动时屏幕投影尺寸稳定；攻击、受伤和死亡的短时形变仍叠加保留。碰撞体、阴影与血条不参与该视觉补偿。ReEchoEditor 编译链接成功，5 项自动化测试通过。
- `Config/DefaultEngine.ini` 的 `EditorStartupMap` 与 `GameDefaultMap` 均设置为 `/Game/Level00`；自动化日志确认成功加载 `Content/Level00.umap`，5 项测试通过。
- 玩家与回响在上一版 280 基础上再乘 0.8，统一世界高度 224；玩家胶囊、阴影、血条和重置高度同步为 27.2/112、0.512×0.5376、64/0.576、112。所有怪物以普通怪物 204 为基准乘 1.2，统一世界高度 244.8，并统一胶囊 34.56/122.4、阴影 0.3264×0.2112、血条 65.28/0.72；Boss 类型只保留贴图与战斗数值差异。ReEchoEditor 编译链接成功，5 项自动化测试通过。
- 玩家、回响、怪物阴影统一采用相对偏移 `-CharacterWorldHeight × 0.28` 与相对缩放 `(0.512, 0.5376, 1)`；回响保持与玩家完全一致，怪物以自身统一高度 244.8 计算相同比例脚下位置，不再使用独立窄阴影或模型半高偏移。ReEchoEditor 编译链接成功，5 项自动化测试通过。
- 剑武器灰盒方块已替换为手绘月牙镰剑 Billboard；近战攻击改用金白色 2D 月牙挥砍贴图，受击反馈改用橙红色星芒贴图。三张透明源图位于 `Content/SourceArt/Effects/`，Cook 资源位于 `/Game/ReEcho/Textures/Effects/`；ReEchoEditor 编译成功，5 项自动化测试通过。
- 武器未显示的根因是其锚点位于相机视线后的世界 +X 方向，被角色 Billboard 完全遮挡；现改为屏幕右侧对应的世界 +Y，并向相机 -X 偏移 8 单位。玩家与回响均使用 `SnapToTargetNotIncludingScale` 和零相对位置附着武器。ReEchoEditor 编译成功，5 项自动化测试通过。
- 武器挂点已与鼠标转向解耦：武器根组件仅继承持有者位置，不继承旋转；移除武器 Actor 自身的挥砍位移/旋转，角色和回响的持剑位置保持固定，攻击方向与月牙伤害判定仍使用持有者朝向。ReEchoEditor 编译成功，5 项自动化测试通过。
- 武器1 Moon Staff 装备独立法杖 Billboard，并发射可见的 `StaffLightWave` 光波投射物：速度 760，按配置射程飞行，使用连续路径检测，只有碰撞敌人才结算伤害。武器2 Crescent Blade 以角色为中心在屏幕平面旋转一周，同时生成 `SlashCrescent`；伤害判定覆盖配置半径内完整 360°，敌人沿角色圆心向外击退。旧即时光束 Actor 已删除。ReEchoEditor 编译成功，5 项自动化测试通过。
- Moon Staff 光波生成点已从角色中心改为法杖 `StaffSprite` 世界位置，并沿固定相机屏幕上方向偏移 105 单位对齐月牙水晶杖头，再沿瞄准方向前移 18 单位避免贴图重叠。ReEchoEditor 编译成功，5 项自动化测试通过。
- 武器2视觉由“绕角色公转”改为“固定手部挂点自转”：攻击期间 `SwordSprite` 位置保持 `SwordSpriteRestLocation`，仅 Roll 角完成 ±360°；`SlashCrescent`、360°范围伤害与径向击退不变。按用户要求未关闭 UE，因此本次仅静态修改，尚未进行 C++ 编译与 PIE 验证。
- 复查确认武器2不显示自身旋转的根因是 UBillboardComponent 在渲染阶段覆盖组件姿态；已将月牙武器视觉替换为使用透明 Sprite 材质的 UStaticMeshComponent Plane，以固定相机基向量保持朝向，并围绕 Plane 法线完成 ±360° 自转。手部挂点、SlashCrescent、范围伤害与径向击退保持不变；按用户要求未关闭 UE，待完整编译与 PIE 验证。
- 武器2背部姿态微调：挂点由相机前侧 X=-8 改为角色后侧 X=8，透明排序优先级降为 -1；静止角度固定为 45°，宽度由 500 缩至 250（原尺寸 0.5）。攻击角度为 45° + Progress×±360°，完整自转后恢复 45°。
- 修复武器2 SlashCrescent 不显示：月牙特效由依赖 Billboard UV 的实现替换为固定相机朝向的透明 StaticMesh Plane，显式绑定 SpriteTexture 材质；按挥砍方向翻转 Plane X 缩放，并将生成深度向相机前移 12 单位，保持 360 世界高度与 0.26 秒生命周期。待关闭 UE 后完整编译和 PIE 验证。
- 武器2中心挂点最终设为相对坐标 (8, 0, 0)：Y/Z 与角色中心对齐，X 沿固定相机视线方向置于角色后方；透明排序保持 -1，45°姿态、0.5尺寸和360°自转不变。
- 武器2连续攻击优化：BasicAttack 增加 Press/Release 持有状态，按住鼠标左键或 J 时每帧尝试出手，由武器冷却统一限频；Crescent Blade 的配置间隔从 0.80 秒调整为 0.18 秒，360°旋转动画同步为 0.18 秒，从而连续衔接且松开立即停止。月牙特效、范围伤害和击退每次有效出手各触发一次。待关闭 UE 后完整编译和 PIE 验证。
- 2026-07-28 完整重编译：强制关闭 UE 后执行 scripts/ue/Build-Editor.cmd；UHT、ReEchoSwordArcActor、ReEchoWeaponActor、ReEchoPlayerPawn 等 11 个动作全部成功，UnrealEditor-ReEcho.dll 链接完成，UBT Result: Succeeded（9.30 秒）。
- 所有关卡怪物尺寸统一放大至上一版的 1.5 倍：世界高度 244.8→367.2，胶囊半径 34.56→51.84、半高 122.4→183.6，阴影缩放 0.512×0.5376→0.768×0.8064，血条高度 65.28→97.92、宽度系数 0.72→1.08。Grunt/Shield/Bomber/MiniBoss/Boss 全部复用该规则，仅贴图和战斗数值保留差异。ReEchoEditor DLL 编译链接成功。
- 怪物视觉改为按源贴图原始尺寸显示：CharacterSprite 设置为 Scale=1，取消 CharacterWorldHeight / TextureHeight 归一化以及 ReEchoBillboardScreenScale 相机深度补偿。不同怪物的宽高直接对应各自 PNG 像素尺寸；攻击/受伤/死亡的短时形变保留。碰撞、阴影与血条继续使用上一版统一1.5倍战斗规则。ReEchoEditor DLL 编译链接成功。
- 相机改为固定正交全景：ProjectionMode=Orthographic、OrthoWidth=11200、宽高比固定16:9，位置(-700,0,900)、角度(-55,0,0)，不再随玩家移动；视口完整覆盖11200×6300场景背景。玩家与回响移除透视深度尺寸补偿，与已调整的怪物一致，在固定正交镜头下移动时保持屏幕尺寸稳定。ReEchoEditor DLL编译链接成功。
- 固定相机进一步收紧：Camera 的 Location/Rotation/Scale 均设置为绝对世界变换；移除 Player Tick 内 UpdateFollowCamera() 调用，仅在 BeginPlay 初始化一次。因此相机组件虽由玩家持有，但不会随玩家位移、旋转或缩放。ReEchoEditor DLL编译链接成功。
- 修复固定相机仍跟随玩家的根因：GameMode 现在生成独立 ACameraActor，配置为16:9、OrthoWidth=11200、位置(-700,0,900)、角度(-55,0,0)，并由 PlayerController SetViewTarget(FixedCamera) 明确使用；玩家 CameraComponent 被停用，菜单恢复时再次锁定 FixedCamera。背景板位置/朝向也只读取 FixedCamera，不再依赖 Player Camera。ReEchoEditor DLL编译链接成功。
- 场景与相机尺寸去硬编码：新增 UReEchoBalanceSettings::ArenaSceneWorldHeight（DefaultGame.ini 默认6300）。背景高度直接读取该配置，背景宽度按纹理宽高比计算；固定相机 OrthoWidth = ArenaSceneWorldHeight × ArenaBackgroundTexture.AspectRatio，AspectRatio也读取实际纹理。修改一个场景高度即可同步改变场景和相机覆盖范围。ReEchoEditor DLL编译链接成功。
- 活动边界与场景尺寸联动：GameMode 根据 ArenaSceneWorldHeight 和背景实际宽高比计算 SceneWorldHeight/Width，并向玩家注入半边界；玩家 Clamp 改为 X=±Height/2、Y=±Width/2。运行时地板和四面碰撞墙使用同一半边界生成，Actor与组件均隐藏但保留碰撞。移除旧 ±1550 玩家Clamp、±1100墙体及成员兜底尺寸硬编码。ReEchoEditor DLL编译链接成功。
- 场景光源改为关卡资产管理：删除 AReEchoGameMode::CreateArena() 中运行时 SpawnActor<ADirectionalLight>、固定旋转和强度代码及相关Include。游戏只使用 Level00 Outliner 中的 DirectionalLight/SkyLight等关卡光源，可直接在Details调整，避免重复方向光警告。ReEchoEditor DLL编译链接成功。

## 2026-07-29 runtime architecture cleanup

- `TotalEncounterCount` in `ReEchoBalanceSettings`/`DefaultGame.ini` is now the single authority for final-encounter gating, Boss spawning, run completion, HUD totals, victory text, and regression tests.
- Removed the obsolete per-frame debug HUD and hidden collision-shape material allocation; the formal UMG encounter HUD remains player-facing.
- Removed unused Niagara and JsonUtilities module dependencies plus the project-level Niagara plugin after confirming no source or asset references; Paper2D remains required by current sprite materials.
- ReEchoEditor build, all five ReEcho automation tests, clean Windows Shipping Cook/Pak/Archive, and a 10-second packaged launch smoke test pass.
