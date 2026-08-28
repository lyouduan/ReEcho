# Plan 144 - 程序 - 狐狸 2D 动画透明边缘与采样修复

## 协调

- Planner 负责人：Codex（当前程序侧 Planner）。
- Executor 负责人：独立 Executor。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Codex`。
- 任务状态：`Ready`。
- 人工验收：`PendingBeforeClose`。
- 本地规划 / 实现基线：`origin/main@a334a04b443c78294f158c667d3829769ceb09c0`。
- 本地实现方式：一任务一 worktree；Planner 与 Executor 分离。
- 依赖 / 阻塞：依赖现有 `Enemy.Fox` Profile 的 `Animation.Born` 与 `Animation.Move` 绑定，不改变其语义或时序。
- Writes:
  - `Content/ReEcho/Art/Animation2D/Enemies/Fox/Born/Textures/*.{png,uasset}`
  - `Content/ReEcho/Art/Animation2D/Enemies/Fox/Walk/Textures/*.{png,uasset}`
  - `scripts/ue/` 下具名狐狸纹理修复/只读审计工具
  - `scripts/ue/audit_plan82_animation_assets.py`（仅在复用现有审计更合理时）
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`
  - `plans/144-fox-2d-alpha-and-sampling-fix.md`
- Stable Reads:
  - `Content/ReEcho/Art/Animation2D/Enemies/Fox/{Born,Walk}/Sprites/*.uasset`
  - `Content/ReEcho/Art/Animation2D/Enemies/Fox/Flipbooks/{Born,Walk}.uasset`
  - `Content/ReEcho/DataAsset/Enemy/Profiles/DA_Enemy_Fox.uasset`
  - `Source/ReEchoPresentation/{Public,Private}/Presentation/Animation2D/`
  - 其他敌人 2D 纹理仅作参数对照
- 影响模式：`Exclusive`（狐狸源纹理与对应 Texture 资产为难合并二进制/美术资源）。
- 兼容承诺 / 下游操作：保持 PNG 尺寸、可见非透明像素、Sprite/Flipbook 引用、帧数/FPS、Pivot、WorldHeight、碰撞和动画语义不变；只修复透明 RGB、Alpha 识别与采样/压缩契约。
- 明确排除：不改狐狸移动、AI、冲刺、攻击、伤害、状态机、Profile、Sprite 源区域、Flipbook 顺序，不改其他角色资产，不通过 Motion Blur/TAA 或全局渲染设置掩盖问题。

## 锁定目标

1. 狐狸出生动画不再因 Born 纹理 Alpha/压缩异常出现白底、白雾或首帧发白。
2. 狐狸移动动画在左右移动、低帧率和分辨率变化下不再因透明边缘白色 RGB、DXT5、World mip 与默认采样组合产生白色模糊边缘。
3. 建立可重复的源图 Alpha bleed 与 UE Texture 设置审计，避免重新导入后回归；不得改变狐狸可见画面主体和动画行为。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEchoPresentation`、`AREA-Presentation`、`AREA-Tests`。
- 对应模块文档：维护 `shared/CODEBASE_MAP/modules/MOD-ReEchoPresentation.md`，补充运行时 2D 角色纹理的透明边缘、Alpha、Mip/Filter/Compression 导入契约；已加入 Writes。
- 设计意图：把问题修在权威源 PNG 与 Texture 导入契约，不在移动代码、Tint、状态机或全局后处理增加补偿。
- 权威状态与依赖：不改变表现状态所有者、公共 C++ API 或模块依赖；源 PNG 仍是对应 Texture 的导入来源，`.uasset` 由 UE 5.8 Editor 重导入并保存。
- 决策记录：透明像素 RGB 使用最近有效轮廓色扩边并保持 Alpha 不变，避免清黑产生黑边；保持 Bilinear 以保护手绘轮廓，关闭角色逐帧纹理 mip 并使用适合 RGBA 的运行时压缩/LOD 组；若实测 Bilinear 仍模糊，必须先用对照 PIE 证明再调整，不直接改为 Nearest。
- 相关文档同步范围：审阅 `shared/CODEBASE_MAP/ARCHITECTURE.md` 与 `shared/CODEBASE_MAP/README.md`，预计拓扑和路由不变；更新 `MOD-ReEchoPresentation.md` 的资产契约。关闭前在本 Plan 记录逐项结论。

## 锁定验收

- [ ] Born 5 张、Walk 24 张源 PNG 的尺寸、Alpha 通道和可见像素保持，透明/低 Alpha 边缘不再携带异常白色 RGB。
- [ ] 29 张对应 Texture 资产由 UE 5.8 重导入并读回统一的 Alpha、NoMipmaps、Clamp、Bilinear 和运行时 RGBA 契约；Sprite/Flipbook/Profile 引用及帧数/FPS 不变。
- [ ] `ReEcho.Presentation.Animation2D`、狐狸纹理只读审计、项目校验、prebuilt check、FullRebuild 和 `git diff --check` 通过。
- [ ] PIE 中狐狸开局 Born 与连续左右移动不再发白模糊，轮廓、色彩、脚点和构图由用户验收。
- [ ] 未提交精选 `GIT_RULES.md` 预构建允许列表之外的 UE 生成物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@a334a04b443c78294f158c667d3829769ceb09c0`。
- 引擎/构建可用性：Git LFS `python scripts/setup_lfs.py --check` 已通过；运行 UE 命令前仍须确认本 worktree 对应 Editor/公共锁空闲。
- 现有聚焦测试结果：只读诊断确认运行时使用 `TranslucentUnlitSpriteMaterial` 与固定 `CharacterTint`；项目使用 FXAA 且关闭 Motion Blur。狐狸 Walk 为 `1024x1024 / DXT5 / TF_Default / TEXTUREGROUP_World / FromTextureGroup mip`，低 Alpha 像素平均亮度约 `208..215/255`；Born 纹理资产出现 `HasAlphaChannel=False / TC_EditorIcon / TF_Default / NoMipmaps` 不一致。
- 共享契约 / 难合并资源风险：29 张 Texture `.uasset` 和源 PNG 属于独占资产；任何远端同路径修改都要求逐资产语义审计，不能选边覆盖。
- 基线损坏时的停止条件：LFS 指针未还原、源 PNG 无法无损解码/重编码、UE 重导入改变 Sprite/Flipbook 引用或可见主体、现有 Animation2D 基线测试失败时停止并分类。

## 实现提纲

1. 对 29 张源 PNG 记录尺寸、RGBA 哈希、Alpha 分布和可见像素哈希，执行确定性 Alpha bleed，仅填充透明 RGB，不改 Alpha 与可见像素。
2. 通过 UE 5.8 Python 重导入对应 Texture，并统一读取/写入 Alpha、NoMipmaps、Clamp、Bilinear、运行时 RGBA Compression/LOD 契约；保存后再次只读读回。
3. 验证 Sprite/Flipbook/Profile 引用、Born `5@10=0.5s`、Walk 帧序/FPS/Pivot/碰撞契约不变。
4. 更新自动审计、模块文档和执行记录，完成构建、自动化与静态门禁；PIE 视觉结果交用户验收。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| 源图 | 狐狸纹理修复工具 `--check` | 29 张尺寸/Alpha/可见像素哈希不变，异常白色透明 RGB 清除 |
| UE 资产 | 狐狸 Texture/Sprite/Flipbook/Profile 只读审计 | 导入参数统一，引用、帧数、FPS、Pivot、碰撞契约保持 |
| 自动化 | `scripts\ue\Run-Automation.cmd -Filter ReEcho.Presentation.Animation2D` | 相关测试通过 |
| 发布构建 | `scripts\ue\Build-Editor.cmd -Configuration Development -FullRebuild` | UHT/UBT 成功并刷新准确精选包 |
| 静态 | `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、`git diff --check` | 项目、预构建和 diff 门禁通过 |
| 人工 | PIE 开局 Born、静止、连续左右移动、低帧率/分辨率对照 | 无白底/白雾/白色模糊边缘，主体色彩与构图保持 |

## 执行记录

### 变化

### 证据

### 剩余风险

### 人工验收结果/请求

- `PendingBeforeClose`：请用户在最终候选 PIE 中观察狐狸开局 Born 以及连续左右移动，确认白色模糊消失且主体轮廓、颜色、脚点和构图未改变。

### 架构文档审阅结果
