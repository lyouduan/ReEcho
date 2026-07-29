# Plan 04 - Gameplay - 2D combat animation feedback

## Locked goal

Add readable attack, movement, hit and death feedback to the current static 2D Billboard actors without changing combat or collision outcomes.

## Locked acceptance

- [x] Player, echo and 2D enemies have subtle idle/movement motion.
- [x] Successful attacks trigger a short visual-only anticipation/lunge/recovery motion.
- [x] Applied damage triggers visual squash/shake without moving the damage collision volume.
- [x] Enemy death visibly shrinks/falls before destruction.
- [x] UE 5.8 Editor build and ReEcho automation pass.

## Implementation notes

- Animate only `CharacterSprite`; actor/collision transforms remain simulation authority.
- Keep static textures now and preserve a future seam for `UPaperFlipbookComponent`.
- Update this plan and project routing/state before commit.

## Evidence

- UE 5.8 `ReEchoEditor` Win64 Development build passed on 2026-07-23.
- `ReEcho.GAS.PlayerAbilityStructure`, `ReEcho.Recording.CapturesWeaponTimeline` and `ReEcho.Recording.InterpolatesAndHolds` passed on 2026-07-23.
- Animation is applied only to each actor's `CharacterSprite`; collision and gameplay transforms remain authoritative and unchanged by squash, shake, bob or lunge.
- Human PIE validation remains required for final timing, amplitude and readability tuning.
- Generated three MushroomGirl idle frames, two attack frames and a 5-column 5120x1024 transparent Sprite Sheet under `Content/SourceArt/Characters/MushroomGirl/`.
- Imported the five frames as cooked `Texture2D` assets and bound the player Billboard to a 4 FPS idle loop plus attack-state frame playback; UE 5.8 Editor build and all four ReEcho tests pass.
- Replaced the player sphere with a root capsule (radius 42, half-height 150), aligned its center with the 300-unit character visual, and adjusted encounter reset height so the capsule base rests on the arena floor; build and four ReEcho tests pass.
- Collision debug drawing now defaults off and is controlled independently with `ReEcho.DebugCollision 0|1`; UE 5.8 Editor build passes.
- Player Billboard now mirrors horizontally from mouse position along the camera-right axis, with a small center dead zone to prevent rapid direction flicker. The first negative-scale implementation was ineffective because UE Billboard rendering discards scale sign; the corrected implementation flips the U/UL texture sampling range and the UE 5.8 Editor build passes.
- Clean Windows Shipping Build/Cook/Pak/Archive passed on 2026-07-23. The final IoStore contains all five MushroomGirl frame `.uasset` and `.ubulk` entries, and `PackagedBuilds/Windows/ReEcho.exe` stayed alive through a 12-second smoke test.
- Added a terminal victory settlement: only clearing the Boss in encounter 6 enters Summary and shows 6/6, time shards, trait count, restart and quit; final-encounter timeout enters Failed. UE 5.8 Editor build and four ReEcho tests pass, including `ReEcho.Run.FinalBossRequiresKill`.
- 2026-07-26：为核心公开类及复杂流程接口补充简洁中文注释；简单 getter、按钮转发和自解释函数不重复注释。
- 2026-07-26：将武器槽位、ID、显示名、攻击间隔、范围、伤害倍率、近战角度和弹体颜色迁移到 ReEchoBalanceSettings/DefaultGame.ini；移除打包不可靠的 Content/Data/weapons.json 运行时文件读取，并增加配置唯一性测试；同时移除 weapons.json 的 RuntimeDependency，仅保留其设计源用途。
- 2026-07-28：将用户提供的三张设定图以确定性白底透明化方式拆成 11 张独立 PNG，并通过 `scripts/ue/import_new_cast_assets.py` 导入 `/Game/ReEcho/Textures/Characters/NewCast`。普通敌人按 SpawnIndex 轮换 6 种外观，第 3 遭遇加入 ClockKeeper 小 Boss；4 名玩家角色通过 `DefaultCharacterId`（J_HEART/J_SPADE/J_CLOVER/J_DIAMOND）配置选择。所有运行时纹理使用构造期硬引用，确保 Shipping Cook 可追踪。
- 2026-07-28：清理2D迁移后的Demo占位Shape：删除玩家隐藏球体、回响隐藏球体/无效幽灵材质更新、敌人Cube/Plane/Cone备用渲染和未使用的球体/通用边界调试函数。玩家/敌人仍以Capsule作为玩法权威，Billboard负责角色表现，StaticMesh平面仅保留软阴影；竞技场、子弹、剑和轨迹等仍可见且承担职责的几何体保留。
### 角色/回响配对资源（2026-07-28）

- [x] 从单张设定表无重绘拆分 5 个玩家与 5 个回响透明贴图。
- [x] 建立 `CharacterId -> Player_* / Echo_*` 一一映射。
- [x] 回响使用录制快照中的角色 ID，避免玩家与回响形态错配。
- [x] Unreal 资产导入、编辑器编译及 `ReEcho.*` 自动化测试通过。
### 场地背景替换（2026-07-28）

- [x] 使用用户确认的无角色/无怪物场地图，不采用生成候选图。
- [x] 导入可打包 `ArenaBackground` Texture2D，并由 GameMode 构造期硬引用。
- [x] 旧灰盒地板与边界墙仅保留碰撞；场景图以零旋转水平铺在地板表面，仅作为地面视觉，不作为 Skybox 或远景板。
- [x] 回响与玩家统一为 350 世界高度、中心对齐 Billboard 和相同脚下阴影参数。
- [x] 水平地面版本完成 ReEchoEditor DLL 链接并通过 5 项 `ReEcho.*` 自动化测试；地面取景比例仍需 PIE 人验。
### 2.5D 地面深度优化（2026-07-28）

- [x] 使用内置图像编辑生成空场地优化版：增强前中后景、环境遮蔽、接触阴影、冷暖光和边缘高低差，不加入角色、怪物或 UI。
- [x] 优化图保存为 `Content/SourceArt/Scenes/ArenaBackground.png`，导入新资产 `/Game/ReEcho/Textures/Scenes/ArenaGround3D`，旧版 `ArenaBackground` 保留回退。
- [x] 场景仍以水平地面平面使用，不作为 Skybox；ReEchoEditor 编译链接和 5 项 `ReEcho.*` 自动化测试通过。
### 天空盒式远景与活动边界（2026-07-28）

- [x] 将 `ArenaGround3D` 从水平地面切换为固定相机使用的 16:9 天空盒式远景板。
- [x] 隐形地板与四面碰撞墙继续承担玩法空间，边界由 ±1400 收紧为 ±1100（中央 2200×2200）。
- [x] 最大敌人出生半径 920 保持在边界内；ReEchoEditor 编译链接及 5 项 `ReEcho.*` 自动化测试通过。
- [ ] PIE 人验远景覆盖、画面透视匹配和边界手感。
### 相机对齐远景板修复（2026-07-28）

- [x] 根据 PIE 截图定位：旧平面中心偏离相机视轴且法线不匹配，导致场地图只在画面顶部显示窄条。
- [x] `StartPlay()` 先取得玩家相机，再创建场景；远景板位于相机 Forward 方向 3000 单位，使用 `MakeFromZY` 令平面法线朝向相机。
- [x] 远景板按 16:9 以 6600×3712.5 世界尺寸留边覆盖；±1100 不可见活动边界不变。
- [x] ReEchoEditor 编译链接及 5 项 `ReEcho.*` 自动化测试通过；仍需 PIE 截图确认最终覆盖。

### 远景贴图一致性修复（2026-07-28）

- [x] 根据纹理编辑器与 PIE 对比，确认 `ArenaGround3D` 源纹理正确，异常来自 StaticMesh 平面的朝向、UV 和视口覆盖方式。
- [x] 用 `UBillboardComponent` 替换远景 StaticMesh，直接保持贴图方向与宽高比，并自动正对固定玩家相机。
- [x] 远景位于相机前方 3000 单位，按 4200 世界高度等比缩放以覆盖不同 PIE 视口；碰撞地板和 ±1100 活动边界保持独立且不可见。
- [x] ReEchoEditor Win64 Development 编译链接成功，5 项 `ReEcho.*` 自动化测试全部通过。
- [ ] PIE 人验最终颜色、覆盖和构图是否与 `ArenaGround3D` 纹理一致。

### 地图消失回归修复（2026-07-28）

- [x] PIE 人验确认大型 `UBillboardComponent` 远景未显示，撤销该渲染方案。
- [x] 改用相机 Forward/Right 基向量对齐的 StaticMesh 平面，使用无光照 `DefaultSpriteMaterial` 直接采样 `ArenaGround3D`，保持纹理宽高比。
- [x] 修正隐藏地板与边界使用的 UE 5.8 Cube 资源路径：`/Engine/BasicShapes/Cube.Cube`。
- [x] ReEchoEditor Win64 Development 编译链接成功，5 项 `ReEcho.*` 自动化测试全部通过。
- [ ] PIE 复验地图显示、方向、覆盖范围与活动边界。

### 白色背景材质修复（2026-07-28）

- [x] 根据 PIE 截图确认平面已生成，但 `DefaultSpriteMaterial` 在普通 StaticMesh 上没有接收到 Billboard 渲染代理提供的纹理，因此显示默认白色。
- [x] 通过 `scripts/ue/create_arena_background_material.py` 生成 `/Game/ReEcho/Materials/M_ArenaBackground` 双面 Opaque Unlit 材质。
- [x] `ArenaGround3D` RGB 直接连接 Emissive，GameMode 构造期硬引用材质以确保 Shipping Cook 收录。
- [x] ReEchoEditor Win64 Development 编译链接成功，5 项 `ReEcho.*` 自动化测试全部通过。
- [ ] PIE 复验地图颜色、方向、覆盖范围和活动边界。

### 固定镜头与五倍远景（2026-07-28，已被后续方案替代）

- [x] 将 `UpdateFixedCamera()` 从“玩家位置 + 偏移”改为固定世界坐标 `(-700, 0, 900)`，旋转保持 `(-55, 0, 0)`。
- [x] 玩家移动不再带动战斗镜头平移，避免远景平面边缘进入视口。
- [x] `BackdropWorldHeight` 从 4200 调整为 21000（5 倍），宽度按 `ArenaGround3D` 原始宽高比同步等比放大。
- [x] ReEchoEditor Win64 Development 编译链接成功，5 项 `ReEcho.*` 自动化测试全部通过。
- [ ] PIE 复验固定镜头构图、玩家可见范围以及四边是否仍会露出场景外侧。

### 受限跟随镜头与两倍远景（2026-07-28）

- [x] 镜头恢复跟随玩家，但 X/Y 跟随焦点分别钳制在 ±450；玩家超过该范围时镜头停在场地边缘。
- [x] 玩家活动边界仍为 ±1100，镜头限制不修改角色移动或碰撞范围。
- [x] `BackdropWorldHeight` 从五倍方案的 21000 调整为 8400（原始 4200 的 2 倍），宽度继续按贴图宽高比等比计算。
- [x] ReEchoEditor Win64 Development 编译链接成功，5 项 `ReEcho.*` 自动化测试全部通过。
- [ ] PIE 复验镜头跟随手感，并在玩家移动至四个活动边界时确认画面不露出远景外侧。

### 镜头边缘穿模修复（2026-07-28，已被世界固定方案替代）

- [x] 根据 PIE 截图与代码核对发现 `CameraFollowLimit` 被误改为 `8400 / 2 = 4200`，远大于 ±1100 玩家边界，导致镜头限制实际失效。
- [x] 镜头跟随焦点限制恢复为独立安全值 ±450；该参数不再从背景美术尺寸推导。
- [x] `ArenaSkyBackdrop` 使用 `KeepWorldTransform` 绑定到玩家相机，始终保持 3000 距离和固定朝向，避免镜头靠近、穿过或离开远景平面。
- [x] 两倍远景尺寸 8400 保持不变；ReEchoEditor 编译链接成功，5 项 `ReEcho.*` 自动化测试全部通过。
- [ ] PIE 在玩家移动至四个活动边界时复验镜头停止位置、角色可见性和背景缩放稳定性。

### 世界固定场景与玩家边界（2026-07-28）

- [x] 移除相机 ±450 焦点钳制；相机以固定 `(-700, 0, 900)` 偏移完整跟随玩家。
- [x] 新增 `ConstrainToArenaBounds()`，每帧将玩家 X/Y 限制在 ±1050，为 ±1100 场地边界和半径 42 的胶囊保留安全余量。
- [x] 解除 `ArenaSkyBackdrop` 与相机的 Attach；远景、怪物和回响均保持世界坐标，镜头移动不会改变怪物相对场景的位置。
- [x] 两倍远景高度 8400 保持不变；ReEchoEditor Win64 Development 编译链接成功，5 项 `ReEcho.*` 自动化测试全部通过。
- [ ] PIE 复验四个玩家边界、镜头完整跟随，以及怪物相对场景位置稳定性。

### 玩家可达角落与地图停止滚动（2026-07-28）

- [x] 玩家活动边界保持 ±1050，可到达场景左下、右下等角落区域。
- [x] `CameraSafeHalfExtent` 独立设为 ±450；相机在中心区域跟随玩家，到取景边缘后停止，玩家继续向角落移动。
- [x] 远景和怪物保持世界固定、不绑定相机，地图到边缘不再继续向外滚动或露出平面外侧。
- [x] 两倍远景高度 8400 保持不变；ReEchoEditor Win64 Development 编译链接成功，5 项 `ReEcho.*` 自动化测试全部通过。
- [ ] PIE 分别走到四个角，复验玩家可达、镜头停止位置和画面不露外侧。

### 地图与角色统一缩小（2026-07-28）

- [x] 远景高度从 8400 缩至 6300（当前方案相当于初始 4200 的 1.5 倍），宽度继续按纹理比例计算。
- [x] 玩家与回响世界高度从 350 缩至 280；玩家胶囊半径/半高同步为 34/140，重置高度为 140。
- [x] 普通怪物、小 Boss、Boss 世界高度分别从 255/300/280 缩至 204/240/224；各类胶囊、阴影按 0.8 同步缩放。
- [x] 玩家与怪物血条高度、宽度同步缩小，保持在对应视觉正上方。
- [x] ReEchoEditor Win64 Development 编译链接成功，5 项 `ReEcho.*` 自动化测试全部通过。
- [ ] PIE 复验角色辨识度、碰撞命中、脚下阴影和血条间距。

### 角色与怪物固定屏幕尺寸（2026-07-28）

- [x] 核对 UE 5.8 `UBillboardComponent` 实现，确认 `bIsScreenSizeScaled` 只限制近距离放大，不能保证远距离尺寸恒定。
- [x] 新增 `ReEchoBillboardScreenScale`，按相机前向 ViewDepth/1000 计算补偿倍率，并限制在 0.35–3.0。
- [x] 玩家、回响、怪物普通/受伤/死亡缩放路径统一叠加补偿；当前角色之间的相对设计尺寸保持不变。
- [x] 补偿只作用于 Billboard 视觉，不改变碰撞体、移动、阴影、血条或伤害结果。
- [x] ReEchoEditor Win64 Development 编译链接成功，5 项 `ReEcho.*` 自动化测试全部通过。
- [ ] PIE 移动镜头并比较近端/远端怪物，复验屏幕尺寸稳定性和动画形变幅度。

### Level00 默认启动（2026-07-28）

- [x] 确认关卡资产存在于 `/Game/Level00`（`Content/Level00.umap`）。
- [x] `EditorStartupMap` 与 `GameDefaultMap` 同时切换为 `/Game/Level00`，编辑器启动和打包程序启动行为一致。
- [x] `GlobalDefaultGameMode` 保持 `/Script/ReEcho.ReEchoGameMode`。
- [x] 自动化日志确认 `MAP LOAD ... Content/Level00.umap` 成功，5 项 `ReEcho.*` 测试全部通过。

### 玩家回响 0.8 与怪物统一 1.2（2026-07-28）

- [x] 玩家与回响从 280 统一缩小至 224（×0.8），两者贴图、阴影尺度完全一致。
- [x] 玩家胶囊半径/半高同步为 27.2/112，血条高度/宽度为 64/0.576，遭遇重置 Z 为 112。
- [x] 所有怪物以普通怪物 204 为基准放大至 244.8（×1.2），小怪、小 Boss、Boss 不再使用不同模型高度。
- [x] 所有怪物统一胶囊半径/半高 34.56/122.4、阴影 0.3264×0.2112、血条高度/宽度 65.28/0.72；类型只影响贴图、属性和行为。
- [x] ReEchoEditor Win64 Development 编译链接成功，5 项 `ReEcho.*` 自动化测试全部通过。
- [ ] PIE 复验玩家与回响视觉一致，以及所有怪物的显示、碰撞和血条高度一致。

### 回响与怪物阴影对齐玩家（2026-07-28）

- [x] 玩家、回响和怪物统一阴影位置规则：`Z = -CharacterWorldHeight × 0.28`。
- [x] 三类角色统一阴影缩放：`FVector(0.512, 0.5376, 1)`，统一使用相对变换。
- [x] 回响高度与玩家相同，因此阴影位置完全一致；怪物按统一高度 244.8 使用相同位置比例。
- [x] 移除怪物构造期 1.5 大阴影和运行时 0.3264×0.2112 窄阴影差异。
- [x] ReEchoEditor Win64 Development 编译链接成功，5 项 `ReEcho.*` 自动化测试全部通过。
- [ ] PIE 复验玩家、回响和怪物阴影是否贴脚、大小一致且无漂浮。
### 月牙持剑攻击与受击特效（2026-07-28）

- [x] 根据参考画面生成月牙镰剑、金白月牙挥砍和橙红受击星芒三张独立透明素材。
- [x] `AReEchoWeaponActor` 移除 Cube 剑身/护手，改用固定世界尺寸的 `UBillboardComponent` 武器贴图。
- [x] `AReEchoSwordArcActor` 移除程序化方块弧段，改用可左右翻转、快速放大的 2D 月牙轨迹。
- [x] `ReEchoAttackEffects` 移除默认 Niagara `SimpleExplosion`，改为短生命周期的 `AReEchoHitImpactActor` 星芒反馈。
- [x] 三张纹理已导入 `/Game/ReEcho/Textures/Effects/`；ReEchoEditor Win64 Development 编译成功，5 项 `ReEcho.*` 自动化测试通过。
- [ ] PIE 复验持剑位置、月牙覆盖范围、左右挥砍方向和受击星芒大小；根据实际画面微调 145/360/112 世界尺寸。
- [x] 修复切到 Sword 后武器被角色遮挡：武器锚点由世界 X 深度方向改为屏幕右侧的世界 +Y，并让玩家/回响以 Snap 规则零偏移附着。
- [x] 武器挂点与鼠标瞄准旋转解耦：固定在角色手侧，只继承持有者位置；挥砍动态由月牙贴图承担，攻击方向不变。
### 法杖光束与月牙旋转攻击（2026-07-28）

- [x] 根据用户参考生成透明 `MoonStaff` 法杖，导入 `/Game/ReEcho/Textures/Effects/MoonStaff`。
- [x] 武器1显示法杖；攻击时自动选择瞄准锥内最近敌人，使用 `AReEchoLightBeamActor` 连接法杖与目标并即时结算伤害。
- [x] 武器2攻击时在屏幕平面绕角色旋转一圈，同时生成既有 `SlashCrescent`。
- [x] 武器1/2显示名更新为 `Moon Staff` / `Crescent Blade`，伤害、射程和冷却继续读取 `ReEchoBalanceSettings` 配置。
- [x] ReEchoEditor Win64 Development 编译成功，5 项 `ReEcho.*` 自动化测试全部通过。
- [ ] PIE 复验法杖头部光束起点、锁敌手感、光束粗细，以及月牙武器旋转半径。
### 光波投射物与360度旋转击退（2026-07-28）

- [x] 生成透明 `StaffLightWave` 光波素材并导入 `/Game/ReEcho/Textures/Effects/StaffLightWave`。
- [x] 武器1从即时光束改为 `AReEchoStaffLightWaveActor`：沿瞄准方向飞行、连续检测路径、命中第一个敌人后结算伤害并销毁。
- [x] 删除已被需求替代的 `AReEchoLightBeamActor`，避免保留错误方案冗余代码。
- [x] 武器2去除前方扇形限制，以角色为圆心对配置半径内所有敌人造成伤害；复用敌人受击逻辑实现径向击退。
- [x] 武器2视觉继续绕角色中心旋转一周，并同步生成 `SlashCrescent`。
- [x] ReEchoEditor Win64 Development 编译成功，5 项 `ReEcho.*` 自动化测试全部通过。
- [ ] PIE 复验光波朝向、尺寸、速度、最大射程，以及360度击退距离和旋转视觉同步。
- [x] Moon Staff 光波起点绑定到 `StaffSprite` 的月牙水晶杖头，不再从角色中心生成。
- [x] 武器2由绕角色公转改为固定手部挂点的贴图自身 ±360°旋转；按用户要求未关闭 UE，待后续关闭后编译/PIE 验证。
- [x] 复查并修复武器2自转不可见：将会强制相机对齐的 Billboard 改为透明材质 Plane，固定在手部挂点并绕自身法线旋转完整一周。
- [x] 关闭 UE 后执行完整 C++ 编译；待在 PIE 中确认武器贴图朝向、尺寸、连续360°自转及月牙表现。
- [x] 武器2移至角色身后，默认倾斜 45°，缩放为原尺寸 0.5；攻击时从静止角完整自转 ±360° 后复位。
- [x] 修复武器2月牙特效可见性：SlashCrescent 改为显式透明 Plane，固定相机朝向、位于角色前景特效层，并保留左右翻转与短时放大。
- [x] 武器2中心与角色中心对齐，并沿相机深度放置在角色身后；不改变旋转和特效逻辑。
- [x] 基础攻击支持按住连续触发；武器2冷却与360°旋转动画统一为0.18秒，消除每圈之间的0.65秒停顿。
- [x] 所有关卡和怪物类型统一按上一版尺寸放大1.5倍，同步碰撞、阴影和血条；ReEchoEditor DLL 编译成功。
- [x] 怪物贴图取消统一高度和相机深度缩放，按源图原始宽高以Scale=1显示；战斗碰撞/阴影/血条规则不变，DLL编译成功。
- [x] 相机改为固定16:9正交全景，OrthoWidth=11200覆盖完整11200×6300背景；取消玩家/回响透视深度缩放，DLL编译成功。
- [x] 相机改为完全绝对世界变换，并移除每帧更新；仅开局初始化一次，确保不跟随玩家，DLL编译成功。
- [x] 使用独立ACameraActor作为PlayerController ViewTarget，停用玩家CameraComponent，并让背景只绑定固定相机；彻底解除镜头与玩家移动关系，DLL编译成功。
- [x] 新增ArenaSceneWorldHeight配置作为场景和相机视野的单一尺寸源；相机宽度按实际背景纹理比例自动计算，DLL编译成功。
- [x] 玩家移动Clamp、隐藏地板和四面阻挡墙统一绑定场景配置尺寸；所有阻挡模型完全隐藏但保留碰撞，DLL编译成功。
- [x] 删除GameMode硬编码方向光，场景照明完全由Level00 Outliner光源控制；DLL编译成功。

### ClockKeeper 商人身份纠正与曝光配置（2026-07-28）

- [x] 从第3遭遇生成和 EReEchoEnemyKind 中移除 MiniBoss，不再配置敌人数值、碰撞与血条。
- [x] ClockKeeper 源图和导入资产改名为 Merchant_ClockKeeper，导入流程清理旧 MiniBoss_ClockKeeper 资产。
- [x] 项目级关闭 Eye Adaptation、Auto Exposure 和 Motion Blur，桌面端使用 FXAA 与100%渲染分辨率。

### 外围随机怪物生成（2026-07-28）

- [x] 第一波普通怪提高到10只，之后每关增加2只、最多18只；爆破怪最多6只，Boss关额外生成8只普通怪和2只爆破怪。
- [x] 出生点根据场景宽高和边缘留白计算，在四条外围边上使用固定种子随机分布，不再写死700/900/1050半径。
- [x] 数量、增长上限和边缘留白均由 ReEchoBalanceSettings / DefaultGame.ini 配置。
- [x] EnemySpawnEdgeInset 删除 C++ 默认数值，外围出生安全距离只由 Config/DefaultGame.ini 写入；运行时代码仅读取配置。
- [x] 伤害数字显式使用 UE 内置透明抗锯齿文字材质，修复 TextRender 在场景中忽略传入橙/红色而显示黑色的问题，并保留透明渐隐。
- [x] 二次修复伤害数字颜色：材质检查确认 AntiAliasedTextMaterialTranslucent 为 Default Lit，仅 UnlitText 将 VertexColor 接入 Emissive；现切换为 UnlitText，并按 UE 官方示例使用 ToFColor(false)。
- [x] 伤害数字朝向改为固定正交相机反向 Forward，不再按每个数字到相机位置的向量旋转，消除画面边缘数字倾斜。
- [x] 怪物出生由场景四边改为玩家周围随机环：角度全随机，距离由 DefaultGame.ini 的 EnemySpawnMinPlayerDistance=450 和 EnemySpawnMaxPlayerDistance=850 控制，并继续按场景安全边界 Clamp。
- [x] 强化敌人受伤 HitStarburst：世界高度112→190，生命周期0.20→0.32秒，透明排序10→50；增加0.2→1.28爆开和0.55快速收缩，使用贴图自身暖橙颜色，仅实际扣血时生成。
- [x] HitStarburst 无显示的根因收敛到 Billboard 渲染代理；改为与 SlashCrescent 相同的透明无光照 StaticMesh Plane，固定相机朝向并显式绑定 SpriteTexture。

- [x] 修复 Windows Cook 的 MCP 端口冲突：关闭 ModelContextProtocol 自动启动，编辑器需要时手动执行 `ModelContextProtocol.StartServer 8000`，打包子进程不再争用 127.0.0.1:8000。

- [x] 2026-07-28 完整 Win64 Shipping BuildCookRun 复验通过：Clean Build、Cook 539 packages、Stage、Pak、Archive 均成功，UAT ExitCode=0；`Packages/Windows/ReEcho.exe` 启动 10 秒仍存活，最新日志无 127.0.0.1:8000 冲突。

- [x] 新增右上角关卡 HUD：显示当前关卡 N/6 与本关剩余整数秒，复用 EncounterDirector 权威计时；最后5秒倒计时变红，ReEchoEditor Win64 Development 编译成功。

- [x] 修复关卡 HUD 在 PIE 不显示：移除可能产生零尺寸的 ViewportSlot 定位，改用全屏 Canvas 根节点和右上角 AutoSize 子面板锚点；重新编译 ReEchoEditor 成功。

- [x] 右上角关卡 HUD 底板改为完全透明，仅保留关卡与倒计时文字；ReEchoEditor 重新编译成功。

- [x] 将 Windows Shipping 流程封装为 `scripts/ue/package_windows.py`：自动关闭 UE、发现 UE 5.8、Clean Build/Cook/Pak/Archive、检查 EXE 并默认执行10秒冒烟测试；支持输出目录、引擎路径、关卡和跳过测试等参数。

- [x] 2026-07-29 端到端执行 `python scripts/ue/package_windows.py` 成功：Clean Shipping Build、Cook 539 packages、Pak/Archive 均通过，生成 `Packages/Windows/ReEcho.exe`，10秒冒烟测试通过。
