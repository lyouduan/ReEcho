# 商店购买符文自动填充空槽

## 批准目标与边界

- 用户于2026-09-01要求：武器符文槽为空时，购买符文后直接装备。
- 基线为最新 `origin/main@531bd9cb08f8808f254de59dd0ff99fe36588a56`，使用独立工作树 `ReEcho-shop-rune-empty-slot-auto-equip` / `fix/shop-rune-empty-slot-auto-equip`；主目录已有两份未提交蓝图不触碰。
- 这是对Plan160购买入口的边界明确修订：只改变“兼容的对应槽尚有容量”时的购入去向，覆盖单级核心和双重非核心第二槽；不恢复满槽挤出旧件，不自动装备异武器符文。
- 继续优先对已有同级装备进行原槽合成；空槽新购的符文如与背包同级合成，结果留在该新增槽。纯卸下/换武器和背包整理不自动填槽，不扫描装备无关库存。
- 不改价格、刷新、份数算法、存档版本、UI蓝图、配表或核心容量；候选装备校验成功后才一起提交装备、所有权、份数和扣费。

## 设计与影响面

- 所有者继续是 `MOD-ReEcho / AREA-Run`；在 `TryResolveRuneInventory` 的有购买AcquiredId路径，先把兼容且对应空槽的新购份数放入候选装备，再调用既有纯合成规划器和WeaponRuntime校验。不调用会挤出旧件的手动装配接口，不增加UI侧玩法逻辑。
- Writes：Run cpp、相关Rune/Shop测试、`shared/CODEBASE_MAP/modules/MOD-ReEcho.md`、`MOD-ReEchoUI.md`、本记录和开发构建的精选Editor包。
- `ARCHITECTURE.md` / `README.md` 的拓扑与稳定标识无变化；模块文档同步新的购买去向。既有Plan160保存为历史批准记录，由本记录说明后续修订。
- 基线完整静态校验已通过，XLSX/CSV同步问题已由最新主线解决；不沿用旧发布例外。本轮只本地实现、验证，未获新发布请求，不推送main。

## 验证目标

- 首个核心/普通符文填入对应空槽；满槽不替换；双槽只填空位，同级购买仍升级原槽；异武器符文保留背包。
- 新增槽合成结果保持装备、卸下仍留包；保存恢复装备及份数；不重复扣费或消费，失败不留下半套装备。
- Development构建、相关Shop/UI/Save/Traits自动化、静态检查与diff检查；实际视觉/交互由用户确认。

## 实施与验证结果（2026-09-01）

- Run购买候选增加兼容性及有效空槽容量判断；原有合成规划、效果编译和原子提交路径保持不变。更新旧空槽购买断言，新增 `ReEcho.Shop.Rune.EmptySlotAutoEquip`，并补上自动装备存读档断言。
- `scripts/ue/Build-Editor.cmd -Configuration Development` 成功；标准UE 5.8，Build ID `55116800`，7模块精选包匹配源码指纹 `82c239d75604a5a02fed9bcea61c486c5dbfcf542423951dc07d705ef46f98e5`。此为本地开发构建，不代替后续发布所需的最终集成FullRebuild。
- 无头自动化 `ReEcho.Shop+ReEcho.UI.Shop+ReEcho.Traits+ReEcho.Run.Save`：52/52通过，退出码0。覆盖单核心/普通空槽、双重第二槽、满槽不挤出、原槽优先融合、新槽融合保持装备、异武器入包、扣费事件原子可见、失败回滚、手动卸下与换武器、存读档及现有商店/UI回归。
- `python scripts/validate_project.py`、`python scripts/ue/prebuilt_editor.py check`、LFS还原/对象检查及 `git diff --check` 均通过。构建/测试共享Unreal锁已按所有者释放。
- 本地证据：`Saved/RuneAutoEquip/build.log`、`Saved/RuneAutoEquip/tests.log`、`Saved/RuneAutoEquip/test-console.log`；日志不提交。
- 已复核架构索引、架构图及模块路由：无新增模块、公开接口、跨模块依赖或稳定标识；只需上述两个模块说明更新。没有修改UI资产、CSV/XLSX、存档格式或主目录用户蓝图。
- 用户测试入口：本工作树根目录 `ReEcho.uproject`。实际PIE视觉/鼠标交互尚待人工确认；当前未推送或合并远端主分支。
