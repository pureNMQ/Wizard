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
