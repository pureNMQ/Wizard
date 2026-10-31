# 物品与金币图标署名

2026-10-31 从 Game-icons.net 下载白色轮廓、透明背景 PNG，保留原图；未修改像素。UE 中作为 UI 纹理导入，金币在 HUD 中乘金色显示。授权为 [Creative Commons Attribution 3.0 Unported](https://creativecommons.org/licenses/by/3.0/)，原许可文本随本目录保存。

| 文件 | 作者与来源 | 项目资源/用途 |
| --- | --- | --- |
| wizard-staff.png | [Wizard staff — Lorc](https://game-icons.net/1x1/lorc/wizard-staff.html) | T_WizardStaff，学徒法杖 |
| round-bottom-flask.png | [Round bottom flask — Lorc](https://game-icons.net/1x1/lorc/round-bottom-flask.html) | T_HealthPotion，生命药水 |
| potion-ball.png | [Potion ball — Lorc](https://game-icons.net/1x1/lorc/potion-ball.html) | T_ManaPotion，法力药水 |
| crystal-cluster.png | [Crystal cluster — Lorc](https://game-icons.net/1x1/lorc/crystal-cluster.html) | T_ArcaneOre，奥术矿石 |
| two-coins.png | [Two coins — Delapouite](https://game-icons.net/1x1/delapouite/two-coins.html) | T_Gold，地面金币与余额图标 |

项目纹理位于 `/Game/Wizard/UI/Icons/`；背包与地面物品共同读取对应物品定义的 Icon，不按物品类型重新猜测图标。本地原图用于保留来源与重新导入，实际资源操作通过 UE-MCP 完成。运行视觉与独立包验证记录在 T05 票据。
