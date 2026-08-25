# Plan102 小地图角色图标

`MinimapIcons/` 保存用户交付的 8 张透明头像原图。运行时资源由
`scripts/ue/import_plan102_minimap_icons.py` 导入到
`/Game/ReEcho/Textures/UI/CombatHud/Minimap/`，并绑定到对应 Player/Echo
Character Presentation Profile。

`_SourceManifest.csv` 是原始文件、稳定 `CharacterId`、Player/Echo 语义、
运行时资产名、尺寸和 SHA-256 的权威映射。不要按文件编号在运行时代码中
建立第二份映射。
