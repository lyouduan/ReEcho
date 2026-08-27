# 开场角色与武器选择 UI 源图

- `Plan132/` 保存正式运行时切图，均由外部交付目录原字节归档，不做重采样或重绘。
- 角色图按稳定 `CharacterId` 命名，武器图按稳定 `WeaponId` 命名；`Selected` 是明亮版本，`Unselected` 是压暗版本。
- `T_UI_Loadout_DescriptionPanel` 与 `T_UI_Loadout_SelectionArrow` 是 Hover/Focus/点击后出现的辅助表现；箭头由每个 `WBP_ReEchoLoadoutEntry` 实例内部复用，确保和对应条目一起缩放。
- 交付目录中的四张 `1-*.png` 是 1920×1080 构图参考，不作为运行时整屏纹理导入。
- 使用 `scripts/ue/import_plan132_loadout_assets.py` 幂等导入到 `/Game/ReEcho/Textures/UI/LoadoutSelection`。
