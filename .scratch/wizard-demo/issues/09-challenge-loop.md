# T09 三波挑战与回营地

- Status: in-progress
- Blocked by: [T03](./03-skills-and-loadout.md)、[T05](./05-drops-and-pickup.md)、[T07](./07-shop-and-trading.md)、[T08](./08-save-and-restore.md)
- Spec: 第 2 节游戏循环；R03、R04、R08、R15、R17、R26、R27、R28、R29，正文见 [规格](../spec.md)
- Updated: 2026-10-30

## 当前阶段

2026-10-29 用户要求提前实现简单战斗循环。第一阶段依赖 T02/T03 已有接口，进入战斗区自动启动、空气墙隔离、胜败立即回营地、三波、行为树两类敌人、GAS 攻击和怪物血条/模型动画。T05/T07/T08 仍为完整循环集成依赖，不阻止此阶段。

## 目标与范围

在既有场景中接入安全营地、开始挑战、两类敌人、三波推进、结算与死亡回营地，整合可重复的完整循环。

## 输入、输出与不变量

- 输入：玩家进入战斗区域、波次配置、敌人死亡与玩家死亡。
- 输出：挑战状态、一次性结算/掉落、重置后的战斗区域与回营地结果。
- 死亡保留金币/背包/装备/搭配，恢复生命/法力；再次挑战重置本轮战斗，循环不中断已确认持久状态。

## 失败与边界

覆盖挑战重复触发、波次无敌人/生成失败、重复死亡事件、技能执行中死亡、掉落残留、重生初始化与嵌套菜单。营地安全和搭配权限取自同一场景业务状态。

## 验收条件

- [ ] 近战追击与远程投射物两类敌人行为可见，数值与波次来自配置。
- [ ] 从营地开始、完成三波、拾取、结算回营地、交易换装、再次挑战的完整循环成立。
- [ ] 死亡取消未结束能力和输入，保留持久状态，正确重建角色/装备并重置本轮战斗。
- [ ] 反复挑战不遗留敌人、掉落、委托或重复效果；安全区与战斗区边界在实际场景验证。
- [ ] 完整暂停/返回/输入恢复及 Windows 独立包的一轮与重复挑战均有证据。

## 验证记录

完整经济/存档循环尚待后续集成；本轮简单战斗阶段的实测见下。

## 未决实现与学习

人数、间隔、配比和奖励作为配置初值试玩调整；挑战状态与生命周期责任落定后记录重要取舍。讲解一轮状态流转和失败恢复路径。

## 2026-10-29 简单战斗阶段交付

- [x] 按用户要求提前战斗闭环；进入区域启动，战斗中空气墙隔离，三波清空或死亡立即回营地。
- [x] 史莱姆近战、独眼怪远程均由同一个 Behavior Tree 的 Pursue → GAS Attack → Wait 运行；模型、Idle/Run、攻击 Montage 与死亡动画匹配 Polyart 骨架。
- [x] 敌人自己的 ASC/AttributeSet 为唯一属性来源；攻击 Ability 提交冷却、前摇、命中 GE，敌弹仅伤玩家；死亡/结束取消 Task。
- [x] Widget Blueprint 配置头顶血条布局，C++ 事件监听生命/上限；PIE 70→35 血量时同步半血。
- [x] 重复挑战、死亡恢复血蓝、保留搭配与冷却、清理所属敌人及弹丸；修复 Pawn 占有改写 Owner、GC 空引用误匹配。
- [ ] 物理 WASD/鼠标/1234/E/Esc 全流程回归、目标地图 Cook、独立 Windows 包内完整挑战尚未执行；本次依验证约定减少打包，不视为最终 T09 完成。

### 工程证据

- 最终结构变更 UHT/UBT：`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor`，退出0，`Saved/T01/20261029-181942-BuildEditor-da4980/result.json`。首轮因并发写头文件造成 UHT 行号变化及一个旧版 API 失败，修复后完整构建通过。
- 自动化：同入口 `-Action Test`，退出0，13/13，0失败/0漏跑；`Saved/T01/20261029-182026-Test-4fd0cf/result.json`。含 Challenge.ConfigurationAndCampRestore、ProjectileCleanupAfterSourceDestroyed、Enemy.DamageShieldDeathAndInvalidTarget、UI.EnemyHealthBarAttributesAndLifecycle，以及原战斗/技能/交互回归。
- 所有资产和地图写入通过 UE-MCP：调用 WizardEnemyAssetTools 两个 Build 函数、WizardChallengeAssetTools.BuildEnemyDefinitions/ConfigureChallengeMap、gameplay.create_blackboard/create_behavior_tree/add_bt_node；保存后 dirty=0。已有布局不重建，只新增 Challenge_Arena。
- 地图配置：中心(2200,0,0)、半径1780、玩家完全入圈140后封墙、营地(-2100,-100,115)。三波人数1/3/5（近战1；近战2+远程1；近战3+远程2），配置可编辑。
- PIE：通过 KismetSystemLibrary.ExecuteConsoleCommand 在 PIE 世界执行 `Wizard.ChallengeSmokeTest`，日志 `Saved/T09/pie-validation.log`，10:24:15 UTC 输出 `WIZARD_CHALLENGE_SMOKE_PASS`；断言64段空气墙Pawn Sweep、三波胜利回营、再入死亡回营、恢复/搭配/清怪。该命令模拟击杀，仅验证状态生命周期。
- 独立于上述Smoke的真实攻击检查：玩家移至(2200,800,100)，史莱姆由(2300,1250)追至(2234,954)，玩家生命100→4，随后自然死亡回营地；截图 `Saved/T09/melee.png`。另在仅PIE实例临时配置一只Beholder、玩家1000生命，真实敌弹命中后1000→916→676；截图 `ranged.png`、`ranged-close.png`。结束PIE丢弃临时波次/生命修改，保存地图仍为三波。
- 初次PIE受后台节流与GPU长停顿影响；临时降低质量并关闭 bThrottleCPUWhenNotForeground 后验证。结束恢复屏幕百分比100、四项质量3、后台节流True；未把低画质写入项目。截图为低画质下功能预览，不作最终视觉/性能验收。
- 首轮 PIE 暴露蓝图默认配置未生效，装配工具补 MarkBlueprintAsModified 后重新编译/保存；后续PIE实际EnemyConfig=DA_Beholder，SourceChallenge正确，血条WidgetClass和绑定控件均有效。

### 双轴结论

Standards：本轮源码检查、UHT/UBT与13项自动化通过；引擎契约采用独立挑战归属而不依赖Pawn Owner，GAS执行与异步Task清理、弱引用/委托解绑、Blueprint动画图和Widget布局均已接入。资产写入符合MCP要求。开发助手仅WITH_EDITOR执行，不在运行游戏时生成资源。

Spec：第2节简单战斗阶段已通过PIE业务验收和两类真实攻击/血条预览；R03/R04物理输入与菜单回归、R15/R17掉落、R26～R28存档/经济集成和独立包未覆盖，保留in-progress。完整验收条件不因本次阶段通过而勾选完成。

学习材料：`.local-learning/T09-challenge-loop.md`。素材实际引用与待核对来源见 `docs/ASSET_SOURCES.md`。

## 2026-10-30 近战追击停止距离修复

- 复现：史莱姆攻击要求中心距离不超过160，但旧 `MoveToActor` 将双方胶囊半径计入接受距离，导致161、180、190距离的站桩玩家仍被导航判为已到达，追击任务无法接近并攻击。
- 修复：`WizardEnemyAI.cpp` 改用 `FAIMoveRequest`，显式关闭Agent/Goal半径补偿，使接受距离使用与攻击相同的中心距离语义；保留原接受距离初值、寻路、部分路径、默认导航过滤器和朝向配置。没有修改地图、蓝图或资产。
- 新增回归 `Wizard.Enemy.PursuitDoesNotArriveOutsideAttackRange`：真实敌人AI、玩家、行为树追击Task与PathFollowing完成事件；检查161/180/190距离不能误报到达，并确认同一玩家在150距离可攻击。夹具不加载导航网格，只验证本次错误的提前到达判定，不代替真实地图路径和实际攻击验收。
- 先失败：`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Test -TestFilter Wizard.Enemy.PursuitDoesNotArriveOutsideAttackRange`，旧实现三个距离均失败，退出1；证据 `Saved/T01/20261030-225505-Test-0e536f/AutomationReport/index.json`。
- 最终编译：`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor`，实际增量C++编译/链接，退出0；`Saved/T01/20261030-225736-BuildEditor-031463/result.json`。本次无反射声明变更，不是Live Coding，也不是干净重编译。
- 修复后回归：`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Test`，15成功、0失败、0未运行，退出0，含新追击回归及既有战斗、技能和挑战生命周期；`Saved/T01/20261030-225817-Test-8ee1c1/result.json`、`AutomationReport/index.json`。
- Standards（引擎API契约/项目约定）：核对本机UE5.4 `MoveTo`/`HasReached`的半径语义，完整增量构建与自动化通过；保留用户已有改动。Spec（第2节近战追击）：本次停止距离缺陷的先失败后通过证据成立；本轮未执行PIE、物理输入或Windows打包，不提升T09整体完成状态。

## 2026-10-31 用户检查与版本交付

- 用户检查后反馈“看不出太大问题”，授权提交并推送当前战斗循环实现与近战追击修复。
- 本次沿用上述实际构建与15项自动化证据；此反馈不替代尚未执行的物理输入矩阵、经济/存档整合及独立包完整循环验收，T09保持in-progress。

## 2026-10-31 Enemy 实现专项审查

- 范围：用户反馈敌人蓝图没有网格体，检查当前工作区源码及编辑态实际资产；不是仅审查相对HEAD的差异。本轮未修改业务源码、蓝图或地图，未运行PIE、构建、自动化或Cook。
- 资产证据：UE-MCP `blueprint.read(includeComponentProperties=true)` 确认 BP_Slime/BP_Beholder 均继承 WizardEnemy，CharacterMesh0 的 SkeletalMesh/AnimClass 均为 None；TargetVisual 均为引擎Cube。`blueprint.get_variable_default(EnemyConfig)` 分别读回 DA_Slime/DA_Beholder，persisted=true、packageDirty=false。
- 两份配置并不相同：DA_Slime 为 Slime_SK/ABP_Slime、生命50、速度240、射程160、攻击间隔1.5、前摇0.333333、bRanged=false；DA_Beholder 为 Beholder_SK/ABP_Beholder、生命70、速度190、射程650、间隔2、前摇0.293333、bRanged=true。各自攻击Montage及死亡动画也不同。两者攻击力12、防御0，共用BT_Enemy及DA_EnemyDrops；共用资源本身不是错误，尚无需求要求奖励必然不同。
- `level.get_actor_details(Challenge_Arena,Waves,world=editor)` 确认三波为 Slime1、Slime2+Beholder1、Slime3+Beholder2。`gameplay.read_behavior_tree_graph` 确认共用树为Pursue→Attack→Wait(0.2秒)，距离及近远程执行差异由配置和C++实现。
- [P2/设计] WizardEnemy.cpp:63-70 在BeginPlay才从配置设置模型、动画类及变换，覆盖蓝图Mesh默认值，且没有编辑态同步。因此蓝图与地图编辑态不能预览最终敌人或可靠调整模型/胶囊/血条关系。建议各敌人蓝图直接保存模型、动画类及组件变换，移除运行时重复覆盖；数值配置独立保留。数据资产驱动表现并非引擎违规，当前缺失编辑态表达才是明确问题。
- [P2/健壮性] WizardChallengeArena.cpp:40 仅校验EnemyClass存在及数量；WizardEnemyConfig没有有效性校验，AI启动也只是静默跳过缺失BehaviorTree。波次允许无EnemyConfig/无行为树的敌人进入战斗，玩家才发现怪物站桩；不一定卡死，但不满足活动敌人的契约。建议在挑战开始前检查实际敌人类默认配置、业务数值和必需行为资源，并给出定位错误；不要将图标等非业务资源机械设为生成的前置条件。
- [P2/设计] WizardEnemy.cpp:34-39 将旧测试靶子的Cube组件强加给全部怪物，且第70行通过FindComponentByClass隐藏第一个静态网格而非显式目标；漏配模型时静默显示测试方块。`level.get_actors_by_class(WizardEnemy,matchSubclasses=true)` 确认地图仍有原生类实例T02_CombatTarget。建议将靶子表现放进专用靶子蓝图，保留地图现有实例与布局，再清理通用敌人上的临时表现。
- Standards：发现配置失败缺少诊断及预检的问题（编码规范第2节）；表现归属/靶子职责是设计判断，不冒称UE强制规则。Spec：当前两种敌人的独立定义与波次引用正确；编辑态表现存在上述问题，本次未重新验收运行时近战/远程效果。T09保持in-progress。
- 调整建议尚未实施；不将本审查记录为修复完成。`git diff --check` 在本轮业务改动前通过；既有工作区改动保留。


## 2026-10-31 删除EnemyConfig并迁入怪物蓝图

- 用户确认采用各怪物蓝图作为配置入口；本轮完成该重构，不改变两种怪物的既有战斗初值或波次配比。
- 删除`UWizardEnemyConfig`及`DA_Slime`/`DA_Beholder`。通过UE-MCP先读出两份DA，给两个怪物蓝图的CharacterMesh0保存SkeletalMesh、AnimClass、位置(0,0,-88)、旋转(0,-90,0)、缩放1，再清空旧引用并以force=false删除DA；随后冷编译新声明，用blueprint.set_class_default逐项迁入13项数值/资源配置，编译保存蓝图。
- Class Defaults的Enemy分类包含Attributes/AI/Attack/Animation/Loot。属性名使用InitialHealth/InitialAttackPower/InitialDefense/InitialMoveSpeed，以区别ASC中的当前属性；BeginPlay仅初始化ASC，不再修改Mesh、动画类或模型变换。AI、攻击Ability、死亡/掉落直接读取当前敌人，伤害上下文SourceObject改为敌人。行为实现仍为C++，未向蓝图事件图复制业务。
- 配置核对：Slime生命50/攻击12/防御0/速度240/距离160/间隔1.5/前摇0.333333/近战；Beholder生命70/攻击12/防御0/速度190/距离650/间隔2/前摇0.293333/远程；弹速700。两个蓝图分别引用匹配的攻击Montage、死亡动画，共用BT_Enemy与DA_EnemyDrops。通过get_variable_default对原DA与新蓝图逐项比较，两者均13/13一致（浮点容差1e-6）；模型、动画类与变换也已读回匹配。两蓝图编译均0错误/0警告。
- 通用敌人移除TargetVisual和隐藏首个StaticMesh的逻辑。保留旧场景靶子本身及变换(-1580,-100,120)：通过level.add_component给该实例保存CombatTargetVisual(Cube，缩放0.7/0.7/1.5，无碰撞)。这只保留现有靶子的表现，不创建额外靶子或重跑场景生成；重启读回旧TargetVisual已消失，仅该实例有CombatTargetVisual。
- 增加IsConfigurationValid校验行为树和有限有效的战斗数值，远程才要求有效弹速。波次拒绝抽象/弃用/旧版类或无有效战斗配置的敌人，并输出类路径。该检查在挑战封墙前执行；独立无行为树靶子仍可受伤但不授予攻击能力。模型、动画和掉落资源仍与业务生成条件分开。
- 现有回归调整：追击测试改为配置实际行为树；挑战配置回归增加拒绝无行为树原生靶子的失败路径；挑战生命周期夹具使用正式Slime蓝图，并在实例上禁用与时序无关的掉落。减速回归不再写死基础速度600，而是验证本次初始速度的50%、不叠乘和到期恢复。
- 长期测试依据（规范6.1）：保留已有追击/GAS效果/结果生命周期测试，覆盖近战无法接近、减速叠乘/无法恢复、重复回营与清理错误；本次只适配公开配置入口并扩展波次预检，不为迁移工具或资产当前数值另建永久断言。资产逐项对比作为本次迁移证据。
- 首轮Test：Saved/T01/20261031-234553-Test-d82532，退出1，22成功/2失败/0漏跑；发现尚未适配的结果生命周期夹具使用原生靶子及减速断言写死600。已按上述契约调整，不隐去初次失败。
- 编译：Saved/T01/20261031-234150-BuildEditor-c61c30，退出0，实际UHT(2 written)+12项构建动作；后续夹具修正增量编译Saved/T01/20261031-234714-BuildEditor-fed67b和20261031-234759-BuildEditor-008421均退出0，末轮5项动作。均非Live Coding、非干净全量重建。

- 第二轮Test：Saved/T01/20261031-234845-Test-788387，进程退出3。生命周期夹具改用正式蓝图后，在WidgetComponent推进时因缺失GameInstance触发引擎断言；已为临时世界补齐GameInstance，未更改游戏血条或行为。修正后BuildEditor：Saved/T01/20261031-235041-BuildEditor-6e6d47，退出0，实际4项增量构建动作。

- 最终统一Test：`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Test`，Saved/T01/20261031-235104-Test-2f0d21/result.json及AutomationReport/index.json，退出0；24成功/0失败/0漏跑，测试事件无警告/错误。涵盖挑战配置/弹丸归属/结果重复生命周期、敌人伤害/追击、技能减速及掉落拾取等既有回归。
- 静态检查：Source/Config/Tools中没有EnemyConfig、WizardEnemyConfig、DA_Slime、DA_Beholder或TargetVisual残余引用；两份DA文件不存在；git diff --check通过（仅既有文件换行提示）。所有资产/地图写入均经UE-MCP，没有一次性C++装配入口。

- Windows目标地图Cook：UnrealEditor-Cmd.exe Wizard.uproject -run=Cook -TargetPlatform=Windows -Map=/Game/Wizard/Maps/L_WizardClearing -unattended -nop4 -NullRHI -UTF8Output -stdout -DisablePlugins=UE_MCP_Bridge -OutputDir=<本轮目录>/Cooked -abslog=<本轮目录>/cook-engine.log。证据Saved/T09/20261031-235151-EnemyBlueprint-Cook/result.json、cook-engine.log；退出0，893 packages，0 errors/0 warnings。输出包含两个BP、匹配的SK/ABP，未包含两份已删DA；没有执行Build/Stage/Package或独立游戏运行。
- 编辑器重新打开后，editor.validate_assets对两个怪物BP和目标地图检查3/3 valid、0警告；compare_class_defaults核对13项，读回生命50/70、速度240/190、距离160/650、间隔1.5/2及近/远程等8项差异，其余共用值一致。波次仍为1、2+1、3+2的原配比。dirty content/map均0。
- Standards：通过本轮源码职责检查、实际冷编译/增量编译、24项业务回归和MCP资产流程；配置与实例状态分离、GAS保持权威、无新增临时工具或计时器。靶子表现局限现有场景实例，不强加给怪物基类。
- Spec：用户确认的蓝图配置入口、DA删除、各怪物原有数值及资源迁移已完成并有编辑态/工程证据；运行时模型、动画衔接、实际近战/远程攻击由用户验收，本轮未启动PIE/游戏，也未用Cook替代效果验收。T09整体保持in-progress。未提交或推送。


## 2026-11-01 使用PBR怪物与专属掉落

- 按用户要求将BP_Slime/BP_Beholder的CharacterMesh0改为MonsterForSurvivalGame/Mesh/PBR下各自SK；OverrideMaterials均为空，使用PBR模型默认材质。模型变换及战斗数值保持本票上一轮已保存值。
- 全部资产写入均经UE-MCP：BS_Slime/BS_Beholder的Skeleton改为对应PBR骨架，两采样替换为Animation/PBR下的IdleBattle/Run；AM攻击段替换为PBR Attack01，并同步Skeleton及Default Section的LinkedSequence；ABP目标Skeleton改为PBR，BP的DeathAnimation替换为PBR Die。沿用原DefaultSlot、Speed输入和C++动画状态，不使用Polyart动画驱动PBR模型。
- 两个ABP及两个怪物BP均编译0错误/0警告。asset.get_dependencies读取8份BP/ABP/AM/BS，共40项依赖；这些资产不再引用Polyart，分别指向PBR模型/骨架/动画。PBR攻击片段长度仍为Slime0.833333秒、Beholder0.733333秒，前摇未调整。
- BP分别引用DA_SlimeDrops与DA_BeholderDrops，四种专属素材定义与概率数量见T05本日记录。旧共享掉落表删除，原矿石定义保留。
- 本轮仅资产/配置/文档改动，没有C++或模块声明变更，沿用2026-10-31最后的实际编译证据；不机械重跑UBT。不为当前模型路径或概率初值创建永久测试；使用编辑态读回、DataValidation及受影响的既有业务回归和Windows地图Cook验证资源依赖。

- 业务回归：`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Test`，证据Saved/T01/20261101-001055-Test-030560/result.json及AutomationReport/index.json，退出0；24成功/0失败/0漏跑，测试事件0警告/0错误。使用现有编译产物验证本轮资产变化，没有新增测试源码。

- Windows目标地图Cook：UnrealEditor-Cmd.exe Wizard.uproject -run=Cook -TargetPlatform=Windows -Map=/Game/Wizard/Maps/L_WizardClearing -unattended -nop4 -NullRHI -UTF8Output -stdout -DisablePlugins=UE_MCP_Bridge -OutputDir=<本轮目录>/Cooked -abslog=<本轮目录>/cook-engine.log。证据Saved/T09/20261101-001248-PBR-MaterialDrops-Cook/result.json、cook-engine.log；退出0，900 packages，0 errors/0 warnings。输出包含两份新掉落表、四定义、四图标及PBR模型/配套动画/材质，不含旧共享表及Polyart模型目录。没有执行Build/Stage/Package或游戏启动。
- Standards：本轮无C++新增或业务分支；资产写入全部经UE-MCP，19项DataValidation与24项现有回归通过，动画骨架/片段引用一致，图标源文件及生成提示词进入工程，git diff --check通过。未引入临时自动化入口或永久配置镜像测试。
- Spec：用户指定的两种PBR怪物及四种专属素材已配置，金币/药水/装备基础掉落仍保留（R15），新图标引用与Cook有效（R16），拾取事件行为沿用既有实现。编辑器已重新打开；实际PBR材质外观、动画衔接、32像素图标辨识度、实际击杀/拾取与独立包运行待用户验收，相关票据保持in-progress。未提交或推送。
