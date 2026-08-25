# Plan 104 - 程序 - 四武器收敛与镰刀补全

## 协调

- Planner 负责人：Codex（当前程序对话内兼任 Planner）。
- Executor 负责人：Codex。
- Plan 编写方（AI 侧）：`Gavyn-side AI`。
- 实现编写方（AI 侧）：`Gavyn-side AI`。
- 任务状态：`Ready`（`Proposed | Ready | InProgress | Review | Closed | Blocked`）。
- 人工验收：`PendingBeforeClose`，需要用户在 PIE/Shipping 验收四武器开局选择和镰刀基础攻击。
- 本地规划 / 实现基线：`origin/main@7c409c81af85255689ff404c38e232b36c0d660c`。
- 本地实现方式：`plan/104-four-weapon-roster`；`C:\Users\gavynqiu\Documents\miniGame\ReEcho-plan104-four-weapon-roster`。
- 依赖 / 阻塞：组合适配已关闭 Plan101；并行 Plan103 将删除迁移期旧 JSON，本 Plan 不读取或维护这些 JSON，最终集成前审计其实际提交。
- Writes:
  - `plans/104-four-weapon-roster.md`
  - `Design/Data/ReEchoData.xlsx`
  - 由工作簿生成的 `Content/Data/{weapon_types,weapons,attack_steps,slot_types,slot_profiles,parts,part_effects,characters,csv_schema,reecho_data_manifest}.csv` 中实际变化文件
  - `scripts/data/sync_xlsx_to_csv.py`、`scripts/validate_project.py` 及聚焦数据测试
  - `Source/ReEcho/{Public,Private}/Data/` 中武器 CSV 契约与读取/校验
  - `Source/ReEcho/{Public,Private}/Weapons/` 中世界执行、镰刀基础规则和法杖/鞭子适配清理
  - `Source/ReEcho/{Public,Private}/Graybox/` 中法杖/鞭子专用载体清理和共享载体收敛
  - `Source/ReEcho/{Public,Private}/Run/` 中四武器商店候选与旧存档兼容判定
  - `Source/ReEcho/{Public,Private}/UI/` 中四武器选择/图标路由
  - `Source/ReEcho/Private/Presentation/` 中法杖/鞭子表现注册清理
  - `Source/ReEcho/Private/Tests/` 中数据、武器、商店、存档、录制、表现回归
  - `Content/ReEcho/DataAsset/Weapon/` 与 `Content/ReEcho/Textures/Effects/` 中经 Unreal 引用审计确认可移除的法杖/鞭子资产和目录绑定
  - `shared/CODEBASE_MAP/modules/MOD-ReEcho.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoWeapons.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoUI.md`
  - `shared/CODEBASE_MAP/modules/MOD-ReEchoVFX.md`
  - 最终 FullRebuild 刷新的 `Binaries/Win64/` 精选预构建包
- Stable Reads:
  - `plans/101-separated-shop-card-pack-refresh.md`
  - `plans/103-remove-legacy-data-json.md`
  - `shared/CODEBASE_MAP/ARCHITECTURE.md` 与 `shared/CODEBASE_MAP/README.md`
  - 保留武器的现有 Niagara、纹理和 Presentation Profile
- 影响模式：`SharedContract`。改变生产武器集合、XLSX/CSV Schema、开局选择、商店候选和旧存档兼容边界，并修改主模块与 Weapons 的世界执行接缝。
- 兼容承诺 / 下游操作：稳定保留 `W_J_01` 长剑、`W_J_04` 镰刀、`W_J_08` 弓、`W_J_09` 枪；不复用 `W_J_02` 或 `W_J_07`。旧存档或恢复快照只要引用已退役 ID，就明确返回不兼容错误，不做静默替换。Plan101 的商店/卡牌刷新状态与 Save v18 迁移保持不变。
- 明确排除：不调整四把保留武器的既有攻速、基础系数和射程（镰刀已明确的外圈规则除外）；不创作新美术；不重解释旧 WeaponId；不删除通用近战、Projectile 或 Run/Shop 基础设施；不修改并行 Plan103 拥有的旧 JSON 和规则清理范围。

## 锁定目标

1. 生产运行时只存在长剑 `W_J_01`、镰刀 `W_J_04`、弓 `W_J_08`、枪 `W_J_09` 四把武器；法杖 `W_J_02`、鞭子 `W_J_07` 及其类型、步骤、槽位、配件、表现和专用执行代码不再进入运行时、开局选择或商店。
2. 镰刀进入开局选择，使用空出的 `InputSlot=1`；长剑、弓保持 `InputSlot=2/3`，枪保持 `InputSlot=5`。开局展示顺序为镰刀、长剑、弓、枪。
3. 猎手和诗人默认武器改为弓，勇者保持镰刀，智者保持长剑；角色描述与实际默认武器一致。
4. 镰刀基础攻击继续使用数据给出的 360 度、150 cm、0.6 系数，并实现描述中已明确的外圈规则：距离攻击中心超过有效半径 50% 的目标额外获得 40% 伤害倍率。比例和倍率由生产 XLSX/CSV 提供，不在 C++ 复制平衡常量。
5. 镰刀开局卡片、商店武器图和手持表现继续使用正式 `Scythe` Presentation Profile/纹理；镰刀符文图标只消费现有已导入资源，不新增主观美术内容，缺少正式 PartId 命名时记录美术交接而不阻塞逻辑。
6. 读取旧存档/录制恢复定义时，`W_J_02`、`W_J_07` 或其他缺失/禁用武器 ID 明确判定不兼容；不得迁移、回退或复用为保留武器。

## 架构影响与设计决策

- 受影响架构标识：`MOD-ReEcho`（`AREA-Data`、`AREA-Run`、`AREA-UI`、`AREA-Presentation`、`AREA-Tests`）、`MOD-ReEchoWeapons`（`AREA-Weapons`）、文档型 `MOD-ReEchoUI`、`MOD-ReEchoVFX`。
- 对应模块文档：维护 `MOD-ReEcho.md`、`MOD-ReEchoWeapons.md`、`MOD-ReEchoUI.md`、`MOD-ReEchoVFX.md`，均已加入 Writes。
- 设计意图：让 XLSX/CSV 的启用武器集合成为唯一玩法真相；Weapons 继续提供资源无关的通用提交/几何，主模块只为四种正式武器装配表现和世界执行。退役武器不以隐藏分支、占位图或已禁用商店行继续存在。
- 权威状态与依赖：武器集合、步骤与镰刀外圈参数由 XLSX→CSV→不可变 Definition 进入 Weapons；最终命中和伤害仍由 Combat 裁决。UI/Shop 从启用武器投影，不持有第二份四武器名单。
- 决策记录：不复用 `W_J_02/W_J_07`，避免旧存档语义漂移；镰刀使用被法杖释放的 Slot1，保留长剑/弓/枪现有输入以减少无关兼容变化；镰刀外圈参数进入数据 Schema，避免按 `Pattern.ScytheSweep` 硬编码数值；退役资源仅在 Unreal referencer 审计为零后删除。
- 相关文档同步范围：必审 `ARCHITECTURE.md`、`README.md`、`MOD-ReEcho.md`、`MOD-ReEchoWeapons.md`、`MOD-ReEchoUI.md`、`MOD-ReEchoVFX.md`；拓扑/索引无变化时只在执行记录写明无需修改原因。
- 关闭前逐项填写审阅结果：
  - `ARCHITECTURE.md`：待审阅模块拓扑与数据链是否变化；
  - `README.md`：待审阅 AREA 路由是否变化；
  - `MOD-ReEcho.md`：待同步四武器数据、商店、存档和表现适配事实；
  - `MOD-ReEchoWeapons.md`：待同步四武器定义和镰刀外圈契约；
  - `MOD-ReEchoUI.md`：待同步四武器选择与图标投影；
  - `MOD-ReEchoVFX.md`：待移除法杖/鞭子阶段性表现说明。

## 锁定验收

- [ ] XLSX 可见生产表、生成 CSV、运行时快照、开局选择和商店只暴露四把保留武器，法杖/鞭子稳定 ID 不被复用。
- [ ] 镰刀可在开局界面选择，进入游戏后显示正确图标/手持资源并能完成基础攻击。
- [ ] 镰刀内圈使用基础伤害，外圈在同一权威命中流程中应用数据定义的额外 40% 倍率；边界与多目标有自动化证据。
- [ ] 猎手/诗人/勇者/智者默认武器分别为弓/弓/镰刀/长剑，描述一致。
- [ ] 含 `W_J_02` 或 `W_J_07` 的旧存档/恢复快照明确失败，Plan101 的当前 Save v18 商店/刷新恢复继续通过。
- [ ] 法杖/鞭子专用 C++、运行时注册、商店候选、测试要求和零引用内容资产已清理；通用 Projectile/Melee 行为未回归。
- [ ] XLSX 同步、聚焦自动化、Shipping Cook/烟测、最终 Development FullRebuild、项目校验、预构建检查和 `git diff --check` 通过。
- [ ] 用户完成 PIE/Shipping 人工验收后才关闭并发布实现；未提交精选允许列表外 UE 生成物或机器本地路径。

## Step 0 门禁

- 基线分支/提交：`origin/main@7c409c81`；包含已关闭 Plan101 和已发布 Plan103 文档。用户已确认组合适配并授权开始。
- 引擎/构建可用性：UE 5.8 与标准构建/打包脚本可用；独占命令前检查交互式 Editor 和 Git common-dir 锁。
- 现有聚焦测试结果：基线 `python scripts/validate_project.py` 通过，但它仍主动要求五把开局武器和法杖爆炸武器，属于本 Plan 要替换的旧契约。
- 共享契约 / 难合并资源风险：`ReEchoData.xlsx` 是二进制共享源；Plan103 将修改 `validate_project.py`、`MOD-ReEcho.md` 和最终预构建包。推送实现前必须吸收并审计其实际结果，重新生成 XLSX/CSV 和全部二进制证据。
- 基线损坏时的停止条件：若保留四武器之一的当前通用攻击、Plan101 商店刷新或 Save v18 基线测试失败，先区分已有问题，不通过削弱断言掩盖。

## 实现提纲

1. 从最新工作簿移除法杖/鞭子的生产表关系，补镰刀外圈字段和四武器输入/默认角色映射，确定性生成 CSV。
2. 扩展资源无关 Weapon Step/Commit 契约携带外圈参数，在统一近战命中路径应用；清理法杖/鞭子专用世界执行、注册和 UI 分支。
3. 让 Loadout/Shop/Save/Recording 只消费四武器集合，并为退役 ID 增加明确不兼容错误与回归测试。
4. 用 Unreal 资产注册表审计法杖/鞭子 Profile/纹理/载体引用，只删除零引用内容；核验镰刀 Profile、纹理、VFX 和开局 UI 加载。
5. 更新聚焦测试、校验器、模块文档和执行记录，完成自动化、Shipping、FullRebuild、静态门禁后交付用户测试。

## 验证矩阵

| 层级 | 命令/检查 | 预期证据 |
|---|---|---|
| XLSX/CSV | `python scripts/data/sync_xlsx_to_csv.py --check` | 工作簿与生成 CSV 无漂移，只有四武器启用 |
| 数据/存档 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Data` 与 `ReEcho.Run` | 四武器关系、退役 ID 拒绝、Save v18 回归通过 |
| 武器 | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Weapons` | 四武器通用行为和镰刀外圈边界通过 |
| 商店/UI | `scripts/ue/Run-Automation.cmd -Filter ReEcho.Shop` 与 `ReEcho.UI` | 四武器候选、开局图标和 Plan101 刷新回归通过 |
| 内容 | Unreal 资产引用/加载审计 | 退役资产零引用，Scythe Profile/纹理/VFX 可加载 |
| Shipping | `python scripts/ue/package_windows.py` | 干净 Cook/Package、CSV staging 与启动烟测通过 |
| 发布 | `.clang-format`、`Build-Editor.cmd -Configuration Development -FullRebuild`、`validate_project.py`、`prebuilt_editor.py check`、`git diff --check` | 最终候选、精选预构建包和文本门禁通过 |
| 人工 | PIE/Shipping 选择镰刀并攻击内外圈目标 | 选择、图标、表现和伤害行为获得用户验收 |

## 执行记录

### 变化

- 2026-08-25：用户确认只保留长剑、镰刀、弓、枪；旧存档引用法杖/鞭子时明确不兼容，并要求镰刀进入开局选择。
- 2026-08-25：审计发现当前生产表仍启用法杖/鞭子；镰刀已有通用扫击、Profile、纹理、VFX、CSV 和符文，但基础外圈额外伤害尚未实现。
- 2026-08-25：用户两次确认组合适配新到达的 Plan101 实现与 Plan103 文档，本任务由拟定 Plan103 顺延为 Plan104。

### 证据

- 规划基线 `validate_project.py` 通过；该结果只证明当前六类型/五开局武器旧契约内部一致，不是四武器验收证据。

### 剩余风险

- Plan103 实现尚未进入远端；它与本 Plan 共享校验器、模块文档和最终预构建包，发布实现前必须重新审计并组合。

### 人工验收结果/请求

- `PendingBeforeClose`：实现候选完成后，请用户验证镰刀开局选择、图标/手持显示、内外圈伤害差异，以及四武器商店候选。

### 架构文档审阅结果

- 待实现后填写。
