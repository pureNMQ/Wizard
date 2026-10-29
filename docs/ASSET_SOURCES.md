# 素材来源与许可核对清单

建立日期：2026-10-28。此表用于追踪项目已有素材及后续发布核对，不把目录名称或素材可加载视为已证明许可。最终交付前由 T10 补齐实际来源页面、权利人、授权条款与所需署名。

| 内容目录/来源标识 | 当前用途或状态 | 已有来源/处理记录 | 发布前待办 |
| --- | --- | --- | --- |
| HandPaintedEnvironment | 当前营地与遗迹场景 | [资产检查](planning/asset-audit-2026-10-27.md)、[场景基线](planning/scene-build-2026-10-27.md) | 核对原获取页面、作者、许可与署名 |
| BattleWizardPBR | 法师、动画与法杖候选 | 资产检查中的网格/骨骼/动画清单 | 核对原包许可与署名，记录实际采用资源 |
| MonsterForSurvivalGame | 敌人模型与动画候选 | 资产检查中的 Slime/Beholder 等记录 | 核对许可，区分实际使用与备用资源 |
| ArrowTrail | 普攻弹丸早期候选，用户因尺寸偏小改用Free_Magic | `/Game/ArrowTrail/FX/NS_ArrowTrail_Magic`保留；早期接入证据见T02 | 补原获取页面、权利人、许可与署名 |
| Free_Magic | 普攻弹丸蓝紫色副本 | 用户复制的`/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_BasicProjectile1_Base`，本步仅调整三个User颜色参数；用户确认替换效果，见T02 | 补原获取页面、权利人、许可与署名 |
| Free_Spells、FXVarietyPack | T03 闪现/护盾/治疗及雷击表现 | Shockwave、Aura_Mana、Buff_Healing；Cascade P_ky_lightning3，实际配置见 Wizard/Data/Skills | 逐包补来源与许可 |
| Free_RPG_Icons_Pack | 技能图标候选 | 资产检查中的纹理类型记录 | 补来源、许可与实际使用清单 |
| Audio/RPG_Fantasy_SFX、Audio/UltimateUIMenusSFX | 从原音效包恢复的 SoundWave | [恢复记录](planning/audio-recovery-2026-10-27.md)；源/目标与摘要在 `SourceAssets/Audio/Recovered/manifest.json` | WAV 转换不改变原素材授权，核对原包许可；SoundCue 图未恢复的边界保留 |
| TopDown、Characters、Cursor、LevelPrototyping、StarterContent | UE 模板、基础场景与临时资源 | 现有项目模板及资产路径 | 标注模板/引擎内容来源和实际交付范围，保留原信息 |
| Wizard | 项目自建地图与两份环境材质 | 场景基线、`Tools/Scene/build_wizard_clearing.py` | 自建材质引用已有纹理，随引用资源一并核对 |
| 计划中的外部物品图标 | 尚未作为本轮新增资源导入 | 规格记录了 Lorc/Game-icons.net 候选 | 导入时保存准确页面、作者、许可文本及署名，不凭候选名称认定已完成 |

`Plugins/UE_MCP_Bridge` 为开发工具，保留原包提供者信息；项目引用限制为 Editor 目标，成品包应通过实际启动日志确认不加载桥接模块。若交付完整工程包含该插件，还需核对上游分发许可。

临时转换工程与工程外备份只属于本地处理证据。T01 验证运行所需资源可被 Cook/打包，T10 完成 R30 的最终来源与许可交付；当前表中的待办不记为已核实。
