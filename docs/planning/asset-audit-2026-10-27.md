# 当前资产对 Demo 的适配检查

检查日期：2026-10-27。项目：Wizard，当前编辑器为 UE5.4。

## 1. 结论

从素材供给看，现有角色、动画、法杖、敌人、场景和特效基本足够完成已确认的 Demo；暂未发现需要重新建模、重新绑骨或跨骨骼重定向的硬性缺口。

需要的主要适配是创建动画组合资产、配置挂点与事件时序、调整特效参数、搭建 CommonUI 布局和导入少量物品图标。这属于正常集成工作，仍需要实际制作与验证。

当前不能直接沿用的明确问题是两套新音效包：共 310 个包文件使用本机 UE5.4 不支持的包头格式。装备/药水图标已有项目外候选，但尚未导入；素材和金币图标仍需补选。

本结论是素材层面的可行性判断，不代表技能、AI、GAS、背包、交易、保存或完整循环已经实现，也不代表最终观感和打包已验证。

## 2. 验证层级

- 文件层：盘点 Content、检查包头格式和部分依赖文件是否存在。
- UE 层：读取 AssetRegistry 的真实类型，实际加载关键网格、动画、特效和代表性依赖，读取骨骼、材质、参数、碰撞等属性。
- 图像层：导出并查看一个技能图标；此前下载的六个物品图标已单独查看。
- 本次未做：修改资产、切换关卡、创建场景对象、运行 PIE、动画/VFX 动态预览、全面材质编译、音效试听或 Windows Cook/打包。

结构检查成功与文件存在，都不能替代实际播放和打包验证。

## 3. 主角与法杖

| 项目 | 实际结果 | 最小适配 |
| --- | --- | --- |
| 法师网格 | `/Game/BattleWizardPBR/Meshes/WizardSM` 可加载，SkeletalMesh，3927 顶点，材质有效，约高 1.97 米 | 配置 Character Mesh 的朝向、胶囊、相机和动画实例 |
| 法师动画 | 33 个 AnimSequence 全部读取成功，全部与网格引用同一个 `UE4_Mannequin_Skeleton` | 创建项目用 AnimBP、BlendSpace、Montage |
| 方向移动 | 前/后/左/右战斗行走均存在，约 1.333 秒；向前跑约 0.8 秒 | 混合局部移动方向与鼠标朝向；侧向/后退高速移动需预览调速或采用简化表现 |
| 攻击/施法 | Attack01、Attack02 Start/Maintain、Attack03 Start/Maintain、Attack04 可加载 | 可复用动作，需确定技能映射与实际出手帧 |
| 辅助动作 | 受击、死亡、喝药、防御、交互、胜利等已读取 | 配置状态转换或 Montage |
| 法杖 | Staff01SM、Staff02SM、Staff03SM 均为独立 StaticMesh，可加载、材质非 fallback | 附着到角色挂点并调整握持偏移 |
| 挂点 | `WeaponSocket` 存在，绑定 `Weapon` 骨 | 预览握持方向和发射点 |

关键限制：33 个法师动画均无 RootMotion、无现成 Notify。原包的展示 BP 使用 AnimationSingleNode 播放 Idle01，AnimClass 为 None；本包目录内未找到现成 AnimBlueprint、BlendSpace 或 AnimMontage。因此有原动画资源，仍需创建游戏动画控制与事件接线。

三根法杖主要沿模型 Y 轴延伸；接口按 Z 轴给出的高度不能用来判断法杖长度。未验证武器穿模和动作观感。

## 4. 敌人

推荐最小组合：同包 PBR 版本的 Slime 近战、Beholder 远程。该角色分工是集成提案，不是素材自带的已完成 AI。

| 网格 | 已读取关键动画 | 兼容结果 |
| --- | --- | --- |
| `/Game/MonsterForSurvivalGame/Mesh/PBR/Slime_SK` | Run、Attack01、Die | 均加载成功，Skeleton 与网格一致；约 686 顶点 |
| `/Game/MonsterForSurvivalGame/Mesh/PBR/Beholder_SK` | Run、Attack01、Attack02ST、Attack02RPT、Die | 均加载成功，Skeleton 一致；约 1533 顶点 |
| Mushroom PBR，作为备用 | AngryVersion 的 Run、Attack01、Die | 均加载成功，Skeleton 一致；约 1021 顶点 |

以上 11 个敌人关键动画均无 RootMotion 和 Notify；需补动画状态、命中/发射时机及 C++ AI。Beholder 没有现成 Socket，可配置发射骨骼/局部偏移，或新增项目挂点。其动作是否适合眼部发射仍待预览。

另有 `Fab/Mini_Simple_Characters___Skeleton__Free_Demo_` 可作候选，但当前主方案无需增加新的骨骼体系。

## 5. 普攻与六类技能表现

| 用途 | 已核实候选 | 调整与边界 |
| --- | --- | --- |
| 普通投射物 | `/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Projectile1` | NiagaraSystem，可加载；挂接 C++ 投射物、控制大小及命中后的生命周期 |
| 火球 | `/Game/ArrowTrail/FX/NS_ArrowTrail_Fire`；备用 `/Game/FXVarietyPack/Particles/P_ky_fireBall` | 前者 Niagara，后者 Cascade；调整颜色、尺寸、拖尾/命中时机 |
| 冰箭 | `/Game/ArrowTrail/FX/NS_ArrowTrail_Ice` | Niagara，Ribbon 启用；调整尺寸与尾迹。减速机制由 GAS 实现 |
| 雷击/电系范围 | `/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Buff_Lightning`；备用 `P_ky_lightning1~3`、`P_ky_thunderStorm` | 可作落点电系爆发候选；尚未确认具有垂直落雷的外观 |
| 闪现 | `/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Shockwave` | 可在出发和到达位置播放短效果，位移由代码实现；未验证专用传送门表现 |
| 护盾 | `/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Mana` | 可作持续光环候选，吸收机制由 GAS 实现；未证明是球形包围罩 |
| 治疗 | `/Game/Free_Spells/VFX_Niagara/NS_Free_Spells_Buff_Healing` | 可加载，依赖含 Cross 材质与 Spiral 网格；调整播放时机与大小 |

7 个代表 NiagaraSystem 均实际加载，系统级结构验证 `valid=true`。代表火/冰拖尾、雷电 Buff、Shockwave、Mana Aura、Healing Buff 的简单发射模块检查分别覆盖 2/2、3/3、4/4、6/6、4/4、4/4。普通 Projectile1 含事件发射器，简单检测只识别 1/6，不能据此认定其余发射器无效。

火/冰效果暴露颜色、尺寸、寿命、长度、发射率等参数；Free_Spells 多个效果暴露颜色、缩放、形状和速度参数，支持常规调整。

`FXVarietyPack` 中实际读取的 fireBall、healAura、thunderStorm、lightning1~3 是 ParticleSystem/Cascade；实现时使用对应类型，不能作为 UNiagaraSystem 引用。

依赖抽查：9 个代表效果的 36 个 `/Game/` 直接材质/网格依赖全部加载；批量读取记录 `dirtiedPackages=false`。第二层 31 个依赖中有一项不存在：`/Game/MyVFX/T_Normal_01`。AssetRegistry 将其记为 `MI_ArrowRibbon_01` 的依赖，但实际读取材质实例时唯一纹理覆盖为存在的 `T_Trail`，未发现该缺失纹理的活跃覆盖。因此当前定为旧依赖/缓存残留疑点，不能据此宣布火拖尾损坏。

循环和持续时长尚未可靠读取：系统忽略了系统级状态字段，相关模块输入工具要求 UE5.5+。具体播放与停止后的尾迹需实际预览，不用结构检查推断。

## 6. 场景与交互物件

`/Game/HandPaintedEnvironment` 在 AssetRegistry 中实际识别为 103 个 StaticMesh 与 3 个 World（Overview、Summer_scene、Winter_scene）。

已有房屋、酒馆、女巫小屋、铁砧、熔炉、祭坛、箱子、木桶、围栏、地砖、树木与岩石等网格，可支持小型营地和固定战斗场地。商店可以使用已有场景交互物件表现，无需以新增商人角色作为开工前提。

实际抽查：

- Anvil 和 Tile_1 均有简单/复杂碰撞、导航碰撞数据，并可影响导航。
- Witch_house 可加载，约 7.54×8.20×11.26 米，材质引用指向包内 Summer_mat/Witch_house。
- 上述三个代表网格的直接材质依赖均存在于项目。

搭建可游玩区域仍需摆放、比例调整、边界/碰撞、NavMesh、出生点、营地和挑战交互配置。103 个网格并非全部完成导航/材质审查；现成展示地图也未验证为可直接游戏的关卡。

## 7. UI、图标与装备外观

- Free_RPG_Icons_Pack 实际识别 50 个 Texture2D，可覆盖火、冰、风、治疗/增益等技能图标候选。
- 实际读取三个代表纹理为 1024×1024；导出查看 Fire_1，属于彩色技能插画。
- 抽查纹理当前为 World LODGroup、Default 压缩、sRGB=false。作为 UI 图标使用时需统一检查和配置纹理设置。
- 当前 `/Game` 目录未识别到 WidgetBlueprint。背包、商店、技能栏和 CommonUI 层级需按项目需求制作，可用 UE 基础控件完成。
- 装备/药水图标的六张 Lorc PNG 当前位于系统临时目录，尚未导入 Content。金币、出售素材及需要的装备变体图标还需补选；这是少量 2D 资源缺口。
- 单色物品图标与彩色技能图标应在 UI 预览中通过底板、尺寸、边框和色彩配置协调，目前没有并置视觉验证。
- 帽子、衣服、靴子的外观替换不作要求，因此缺少这些独立 3D 网格不会阻塞装备属性系统。

## 8. 音效包：已确认的版本问题

| 包 | 磁盘包文件 | UE5.4 注册结果 | 包头检查 |
| --- | --- | --- | --- |
| RPG_Fantasy_SFX | 100 | 目录查询 0；代表 Spell/8__1_ 加载失败 | 全部 LegacyFileVersion=-9 |
| UltimateUIMenusSFX | 210 | 目录查询 0；代表 CasualMobile_01 加载失败 | 全部 LegacyFileVersion=-9 |

UE5.4 本机 `Engine/Source/Runtime/CoreUObject/Private/UObject/PackageFileSummary.cpp` 第 107～120 行只支持到 `CurrentLegacyFileVersion=-8`；读取 -9 时重置版本并拒绝继续正常加载。因此日志虽显示 `too old / Package Version: 0`，不能据此把根因解释为音效真的来自过老引擎。

证据位置：`Saved/Logs/Wizard_2.log` 的音效扫描告警，以及本次加载的 3644、3654 行附近。原始文件前 24 字节读取分别显示 Spell 样本 `(-9, 864, 522, 1017, 0)`、UI 样本 `(-9, 864, 522, 1018, 0)`；其中后续字段仅作原始数值记录，不凭此推断确切保存引擎版本。

最小处理方向：取得 UE5.4 兼容版本，或取得原始 WAV 后在 UE5.4 中重新导入所需音效。当前 Content 未发现 WAV/OGG/MP3 源文件。此次没有改包头、迁移或重导入。

StarterContent 中另有 24 个可识别 SoundWave/SoundCue，已实际读取 Explosion01（约 3.17 秒）、Fire01（约 6.20 秒）作为临时声音候选；它们不能替代对整套 RPG/UI 音效适用性的试听确认。

后续恢复验证：已用独立 UE5.8 临时项目导出全部 205 条音频，并在临时 UE5.4 项目验证两条样本往返 PCM 一致；105 个 SoundCue 的节点图尚未恢复，主项目原资产保持不变。详见 [音效恢复验证](audio-recovery-2026-10-27.md)。

## 9. 最小适配清单与后续验证

1. 主角和两个敌人使用各自匹配的动画，创建项目动画组合资产及攻击/施法时序。
2. 附着已有法杖，校准握持与发射位置。
3. 七类效果采用现有资源，调整缩放、颜色、朝向、播放/停止时机；雷击、闪现、护盾允许采用前述替代表现。
4. 用现有场景网格搭建小营地与战斗区域，并配置碰撞/导航。
5. 制作 CommonUI 布局，导入装备/药水/金币/素材图标并统一纹理设置。
6. 重新取得可被 UE5.4 读取的必要音效；无需先凑齐两包全部内容。
7. 在最小战斗原型中验证方向混合、握持、出手帧、特效实播和俯视角可读性，再验证 Cook 与独立包。

就当前基础要求而言，继续大量搜集角色、动画和场景包的收益有限。优先完成上述集成验证，可以更早确认真正影响交付的缺口。
