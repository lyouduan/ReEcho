# 玩法数据迁移说明

本目录不再保存运行时表格。Plan 146 起，玩法与音频配置的唯一权威是 Unreal Editor 中的蓝图数据资产：

- 总入口：`/Game/ReEcho/DataAsset/Gameplay/DA_ReEchoGameDataCatalog`
- 角色与属性：`DA_ReEchoCoreData`
- 卡牌：`DA_ReEchoCards`
- 元素、状态与反应：`DA_ReEchoElements`
- 武器、攻击段与配件：`DA_ReEchoWeapons`
- 怪物、战斗参数与掉落：`DA_ReEchoEnemies`
- 关卡、遭遇、波次与出生：`DA_ReEchoEncounters`
- 商店：`DA_ReEchoShop`
- 音频事件：`/Game/ReEcho/DataAsset/Audio/DA_ReEchoAudioEvents`

策划应在 Unreal Editor 内容浏览器中编辑对应资产并保存；重新开始 PIE 或重启项目后即可读取新值，不需要编译 C++，也不需要运行表格同步脚本。稳定 ID、引用关系和注册行为仍由运行时加载及 `ReEcho.Data.Asset` 自动化校验。

旧 XLSX 只作为 Git 历史迁移证据，不再是生产事实来源。仓库与 Shipping 包禁止包含 CSV。
