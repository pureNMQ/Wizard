# 素材来源与许可核对清单

建立日期：2026-10-28。此表用于追踪项目已有素材及后续发布核对，不把目录名称或素材可加载视为已证明许可。最终交付前由 T10 补齐实际来源页面、权利人、授权条款与所需署名。

| 内容目录/来源标识 | 当前用途或状态 | 已有来源/处理记录 | 发布前待办 |
| --- | --- | --- | --- |
| HandPaintedEnvironment | 当前营地与遗迹场景 | [资产检查](planning/asset-audit-2026-10-27.md)、[场景基线](planning/scene-build-2026-10-27.md) | 核对原获取页面、作者、许可与署名 |
| BattleWizardPBR | 法师、动画与法杖候选 | 资产检查中的网格/骨骼/动画清单 | 核对原包许可与署名，记录实际采用资源 |
| MonsterForSurvivalGame | T09：2026-11-01改为PBR Slime近战、Beholder（观察者）远程，对应PBR IdleBattle/Run/Attack01/Die；项目ABP/BlendSpace/Montage位于Wizard/Enemies/Animation | 2026-10-29 Polyart早期预览；2026-11-01经UE-MCP改用PBR骨架/动画/材质，编辑态编译及资源验证见T09；PBR效果待用户验收 | 原包来源与许可由 T10 核对 |
| ArrowTrail | 普攻弹丸早期候选，用户因尺寸偏小改用Free_Magic | `/Game/ArrowTrail/FX/NS_ArrowTrail_Magic`保留；早期接入证据见T02 | 补原获取页面、权利人、许可与署名 |
| Free_Magic | 普攻弹丸蓝紫色副本 | 用户复制的`/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_BasicProjectile1_Base`，本步仅调整三个User颜色参数；用户确认替换效果，见T02 | 补原获取页面、权利人、许可与署名 |
| Free_Spells、FXVarietyPack | T03 闪现/护盾/治疗及雷击表现；T05早期星点副本保留 | Shockwave、Aura_Mana、Buff_Healing；Cascade P_ky_lightning3；NS_LootGlow保留用户调整，最新掉落已移除其组件 | 逐包补现用资源来源与许可 |
| Free_RPG_Icons_Pack | 技能图标；曾作T04背包验证占位 | T04曾用Holy_1/Water_1/Earth_1/Fire_1；T05四种物品定义现已改为下列Game-icons.net图标，保留技能引用 | 补现用技能图标来源/许可；物品新图标视觉按T05验收 |
| Audio/RPG_Fantasy_SFX、Audio/UltimateUIMenusSFX | 从原音效包恢复的 SoundWave | [恢复记录](planning/audio-recovery-2026-10-27.md)；源/目标与摘要在 `SourceAssets/Audio/Recovered/manifest.json` | WAV 转换不改变原素材授权，核对原包许可；SoundCue 图未恢复的边界保留 |
| TopDown、Characters、Cursor、LevelPrototyping、StarterContent | UE 模板、基础场景与临时资源 | 现有项目模板及资产路径 | 标注模板/引擎内容来源和实际交付范围，保留原信息 |
| Wizard | 项目自建地图与两份环境材质 | 场景基线、`Tools/Scene/build_wizard_clearing.py` | 自建材质引用已有纹理，随引用资源一并核对 |
| Wizard/VFX/M_LootBeam、M_LootGroundGlow | T05装备细金色光柱与全部掉落底部柔光 | 项目自建程序材质，无外部贴图，配合UE引擎Plane；参考用户提供的掉落效果截图制作，未导入截图像素 | 实际比例/亮度与成品加载待验收 |
| Game-icons.net 物品与金币图标 | T05通过UE-MCP导入5张透明PNG，替换四种物品占位图标并提供金币图标 | Lorc：Wizard staff、Round bottom flask、Potion ball、Crystal cluster；Delapouite：Two coins；均CC BY 3.0，准确来源与许可见[署名](../SourceAssets/Icons/GameIcons/ATTRIBUTION.md) | T05运行视觉与独立包加载待验；T10交付时保留署名和许可 |
| Game-icons.net 装备图标 | T06 帽子、衣服、靴子三张透明PNG，经UE-MCP导入 | Lorc：Pointy hat、Robe、Boots；CC BY 3.0；原图、来源链接与许可见上述署名目录 | 用户验收小尺寸辨识度；T10保留署名与许可 |
| Wizard/UI/Icons/T_SlimeMucus、T_SlimeEye、T_BeholderTentacle、T_BeholderEye | T05两类怪物的四种专属素材图标 | 2026-11-01按用户授权用内置image_gen分别生成透明PNG，原图及完整提示词见[生成记录](../SourceAssets/Icons/GeneratedMonsterMaterials/GENERATION.md)，经UE-MCP导入 | 运行时小尺寸辨识度与成品加载由用户验收 |

`Plugins/UE_MCP_Bridge` 为开发工具，保留原包提供者信息；项目引用限制为 Editor 目标，成品包应通过实际启动日志确认不加载桥接模块。若交付完整工程包含该插件，还需核对上游分发许可。

临时转换工程与工程外备份只属于本地处理证据。T01 验证运行所需资源可被 Cook/打包，T10 完成 R30 的最终来源与许可交付；当前表中的待办不记为已核实。
