# T12 战斗表现与打击感

- Status: in-progress
- Blocked by: 无；复用 T02/T03/T09 已有接口，其未完成验收仍保留。
- Spec: R01～R08、R29、R30、R33、R34 与规格战斗表现补充。
- Updated: 2026-11-02

## 2026-11-02 玩家死亡动画

用户确认特效大体没有问题，继续指出玩家没有死亡动画；此反馈不等于完整循环、性能和生命周期全部验收通过。

- 根因：OnHealthChanged仅取消能力/清理状态，ABP_WizardCharacter最终输出只有移动与上半身攻击混合，没有死亡动作入口。现有BattleWizardPBR/DieAnim长0.833秒，可直接用于玩家骨架。
- 新建项目AM_WizardDeath，DeathSlot、0.08秒淡入、关闭自动淡出，倒下后保持末姿势。动画蓝图在原LayeredBoneBlend之后接全身DeathSlot，再输出Root，原普攻/技能上半身Slot不变。BP_WizardCharacter引用该Montage，资产全部经UE-MCP保存。
- C++ Character只负责幂等播放/停止死亡表现和死亡瞬间清零移动速度；PlayerState在取消能力后按真实生命状态同步，Avatar初始化同步当前状态，RestoreAtCamp恢复生命的原回调停止Montage。回营传送顺序、2秒结算、输入规则、护盾/伤害/掉落均未改。
- Standards：实际UHT/UBT通过`Saved/T01/20261102-025149-BuildEditor-69a641`，新增测试编译通过`025319-BuildEditor-312006`；两份蓝图编译0错误/告警。反馈测试6/6通过`025339-Test-aeb276`，挑战回归3/3通过`025412-Test-b4a5d3`，均0失败/告警。新增PlayerDeathRecovery通过真实玩家蓝图、动画实例、ASC血量回调和RestoreAtCamp检查连续两次死亡/恢复、重复通知不重播、关闭自动淡出；NullRHI工程检查不代表实际姿态像素验收。该测试保护跨生命周期残留与不能再次死亡的风险，不锁定美术帧或颜色，符合规范6.1。
- Spec：已接通死亡表现与回营恢复；跑动/施法中倒地、末姿势与地面接触、回营后实际操作待用户试玩。未启动PIE或游戏。
- 阶段集中Windows打包通过：`Saved/T01/20261102-025441-Package-33068c`，Game目标实际编译，Build/Cook/Stage/Package/Archive成功、退出0，Cook汇总0错误0告警（启动仍有既有UTS告警）。本次同时纳入此前普攻配色、减速霜雾和金色护盾修订；UAT/Manifest_UFSFiles_Win64.txt包含AM_WizardDeath、M_IceMist、NS_SlimeDeathParticles。成品`Saved/Packages/20261102-025441-Package-33068c/Windows/Wizard.exe`。编辑器已恢复并读回死亡引用/保持末姿势配置；未运行成品验收。

## 2026-11-02 单项修复：金色护盾及移动包裹余量

用户反馈移动时穿出小盾球，并要求改为金色；按盾体改色处理，未改玩家模型。

- 盾体原本已直接附着角色根组件，无插值延迟；旧横向半径为胶囊半高0.8倍、竖向1.1倍，跑动姿态余量不足。新增Presentation.ShieldRadiusScale配置为(1.25,1.25,1.35)，较旧尺寸横向+56%、竖向+23%；使用引擎球体50厘米半径换算，保留无碰撞和直接根组件附着。
- 经UE-MCP修改M_CombatShield发光色为(3,1.65,0.12)，同步DA_ShieldPresentation的Flash/Sparks1/Spiral1/Circle为金色。开启、持续与到期粒子一致改色；球体基础透明度/Fresnel保持，护盾数值、持续时间与地图不变。
- Standards：真实UHT/UBT通过，`Saved/T01/20261102-023811-BuildEditor-7a56fe`，28项编译/链接动作、退出0；`-Action Test -TestFilter Wizard.Combat.Feedback`通过5项、0失败/告警，`Saved/T01/20261102-023906-Test-65ae12`。已有CueEffectContext补充目标平移/转向后盾体中心同步与无碰撞断言，延续生命周期测试，不锁定美术尺寸；不新增独立测试入口。重开编辑器后通过MCP保存尺寸配置并读回金色材质与颜色参数。
- Spec：尺寸与金色修改完成；真实跑动、转向、施法姿态是否完整包裹，以及群战金色亮度由用户验收。没有启动PIE或成品，本轮未打包；旧独立包不含本次修改。T12保持in-progress。

## 2026-11-02 单项修复：减速改为附着霜雾

用户确认将冰箭减速表现从地面冰刺改为霜雾。本轮仅改Status表现。

- UE-MCP修改现有NS_IceStatus：关闭Mesh1、Mesh2、Trace、Wave的渲染器，只保留Smoke与Sparks1；该系统仅被DA_IceArrowPresentation引用。
- Smoke改用项目副本M_IceMist（源为Free_Magic/M_Free_Magic_Smoke），保留粒子颜色、纹理与DepthFade，将原Opacity乘0.22、Emissive乘0.4，降低遮挡。未更改共享烟雾材质或冰箭全局ColorOverrides，避免影响投射物/命中颜色。
- Status相对敌人根组件偏移Z从-80改为-15厘米，缩放从(0.18,0.18,0.18)改为(0.12,0.12,0.18)。仍沿用减速GE的3秒与持续Cue移除清理；Status.Lifetime不控制附着效果销毁，没有以其4秒改写减速时长。
- Standards：资产全部经UE-MCP修改/保存，14个Niagara脚本编译通过、0失败；六个Renderer启闭、Status配置、材质输出连接读回正确，非Status的4个Stages逐项比较未变。没有C++、伤害、减速数值或地图修改，不新增长期测试、不重跑UBT。
- Spec：地面冰刺/冲击波/痕迹渲染已关闭；霜雾贴合、持续可见性、重复命中刷新及状态结束消失的实机观感仍待用户验收。未启动PIE，未重复打包；现有独立包不含本次变更，新材质Cook纳入后续集中打包。T12保持in-progress。

## 2026-11-02 单项修复：普攻紫色一致性

用户反馈普攻弹道紫色，命中却蓝绿交替。本轮仅处理普攻色彩及史莱姆死亡叠加遮盖。

- UE-MCP读取DA_BasicPresentation.ColorOverrides：Flash/Sparks1原为(0.5,1.7,3)，确为蓝青色；配置检查R>G且B>R修复前失败。改为(1.35,0.24,3)，匹配现有紫色拖尾的RGB比例，补全Smoke/REF/Trail覆盖；REF(35,8,100,1)、Trail(22.5,4,50,2)保留资产原亮度/透明度。命中敌人和撞墙共用此配置。
- 绿色叠加来源：Report在致死结算先播放史莱姆Death（绿色NS_Death、0.6缩放），投射物随后在同点播放Impact（0.25缩放）。普通Hurt没有粒子。由此判断死亡大闪光会与普攻爆点混合；未冒充实机像素验证。
- NS_Death亦供玩家和观察者使用，因此只复制为NS_SlimeDeathParticles，关闭Flash渲染器、保留Sparks1，仅替换史莱姆Death引用。原共享系统、其他怪物、六技能均不改。
- Standards：全部资产修改经UE-MCP，项目副本保存；没有改C++、结算、输入、地图。Niagara真实编译6脚本UpToDate、0失败；读回6项颜色均为紫色范围，死亡Flash关闭、Sparks1开启。纯表现参数不新增长期测试，不重跑UBT/业务自动化；本次未打包，新副本Cook留待阶段集中打包。
- Spec：配置检查已通过，用户在编辑器复验连续非致死命中、撞墙和击杀时的颜色一致性待完成；绿色死亡碎粒仍保留，但不再有大片绿色死亡闪光。既有独立包不包含本次修改。T12保持in-progress。

## 已确认范围
强爆发视听、实际结算飘字、分级震屏、短全局顿帧；六技能机制及数值不变，不新增击退/硬直/暴击。质量优先，允许延后原11月3日交付。运行时验收由用户执行。

## 验收
- [ ] 普攻、六技能、史莱姆与观察者出手/命中/状态/死亡反馈完整。
- [ ] 飘字只显示实际生命损失、吸收、有效治疗；范围伤害不叠加镜头冲击。
- [ ] 暂停、失焦、死亡、回营地及世界退出无顿帧/循环特效残留。
- [ ] 动画、声音、特效同步且群战可读；三轮挑战无明显性能退化。
- [x] UHT/UBT、受影响工程测试、集中Windows打包通过。
- [ ] 用户运行效果与完整循环验收。

## 验证记录
2026-11-02：首轮代码、资产和工程验证已交付，用户运行效果验收尚未执行，状态保持in-progress。

- 关联：[T02](02-basic-combat.md)、[T03](03-skills-and-loadout.md)、[T09](09-challenge-loop.md)，不关闭其原有未验收项。
- Editor实际UHT/UBT：`pwsh -NoProfile -File Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor`，结构变更成功证据`Saved/T01/20261102-011224-BuildEditor-d69397`；最近增量编译`Saved/T01/20261102-011327-BuildEditor-bba20f`，退出0，非Live Coding。
- 全量工程回归：同入口`-Action Test`；`Saved/T01/20261102-011342-Test-a6ae66/AutomationReport/index.json`，39 succeeded / 0 failed / 0 notRun，退出0，包含新增3项Wizard.Combat.Feedback测试。
- Windows打包：同入口`-Action Package`；`Saved/T01/20261102-011457-Package-1e982c/result.json`，退出0，Editor/Game Build、Cook、Stage、Package、Archive完成，Cook汇总0错误0告警。启动时工具链另有Unreal Trace Server启动告警，不是Cook资产错误。成品`Saved/Packages/20261102-011457-Package-1e982c/Windows/Wizard.exe`；未启动成品做运行验收。
- UE-MCP编辑态：11份Presentation读回成功；16个SoundCue（15个新图及1个已验证图的复制），WavePlayer→Modulator连通，并发普通8/高优先6。3个新增GC的Tag正确。5份Cue/HUD蓝图编译0失败0告警，HUD有Overlay根。观察者弹丸12个Niagara脚本UpToDate、0错误；材质图输出连接在编译后保留。原始摘要`Saved/T12/asset-readback.json`。
- 初轮编译发现头文件顺序、TObjectPtr推导、Shake构造和前向声明问题，已修复并重新实际编译通过；初轮结算测试失败因夹具未注册属性集，修正注册后通过。失败日志留在Saved/T01，不混作通过证据。
- 地图只新增Challenge_Arena.Presentation引用；保留用户未提交改动和手工布局。没有启动PIE、游戏或额外运行效果检查命令，没有新增临时反射桥接和一次性装配工具。
- 配置说明、用户试玩清单和限制：[战斗表现](../../../docs/COMBAT_PRESENTATION.md)。学习摘要：`.local-learning/T12-combat-feedback.md`（本地忽略）。

### 长期测试取舍（规范6.1）

Settlement保护实际生命损失、吸收、溢出治疗与尸体/初始化不冒伤害，防止显示理论数值。HitStopLifecycle保护同帧最大请求、进行中不延长、真实时间恢复、触发间隔及解除占有恢复原倍率，防止永久慢动作。NoBusinessSideEffects保护无配置/取消反馈不改变外部时间。均通过公开边界验证规则，不锁定美术尺寸或私有调用次数，值得随业务长期维护。

### 双轴审查

Standards：核心行为C++、资源由资产配置，GAS独立结算，Cue/局部反馈有生命周期清理，资产变更经UE-MCP；实际构建、39项测试、Windows打包及git diff空白检查通过。程序材质和素材副本来源已补记录，原素材许可最终核对仍属T10。

Spec：保留六技能机制、数值、输入和场景布局，按用户确认加入实际数字、分级震屏和短顿帧；普攻、技能、两类敌人与挑战已接入阶段表现。动画接触感、特效缩放/色彩、音效听感、群战可读性、失焦实际操作和三轮挑战性能仍待用户验收。没有将工程测试或Cook通过当作运行体验通过。

## 2026-11-02 用户首轮试玩与Cue分发修复

用户确认伤害飘字、震屏及观察者弹道可见；反馈观察者听不到声音，普攻/火球/冰箭及观察者弹丸无命中特效，雷击/闪现/护盾/治疗无特效。这里只确认上述可见现象，不提前勾选全部数字真实性、生命周期和完整循环验收。

共同根因：四个继承原生父类标签的Cue蓝图资产名不能推导完整GameplayCue标签，UE5.4的`DeriveGameplayCueTagFromClass`回退了父类GameplayCueTag，却留下GameplayCueName=None。管理器通过GameplayCueName扫描，因此Burst、Windup、Skill.Impact、Skill.Shield缺席实际分发表；直接生成的弹道与玩家侧数字/震屏不依赖这条链路。此前仅检查Tag和蓝图编译，未覆盖真实分发注册，是首轮工程检查的缺口。

- 先增加`Wizard.Combat.Feedback.CueDiscovery`，检查引擎实际RuntimeCueSet能为五类战斗标签找到并加载对应处理类；不启动世界、PIE或音画播放。长期保留依据（规范6.1）：资源重命名/父类继承可能导致整条音画反馈链静默失效，标签和编译检查发现不了；断言公开分发边界，不锁定粒子参数或资产具体路径。
- 实际编辑器编译：`pwsh -NoProfile -File Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor`，退出0，`Saved/T01/20261102-013431-BuildEditor-5cfa70`，新增测试源码实际编译，未使用Live Coding；无反射声明变更需要重新生成。
- 修复前复现：同入口`-Action Test -TestFilter Wizard.Combat.Feedback.CueDiscovery`，退出1，`Saved/T01/20261102-013451-Test-77ba8d/AutomationReport/index.json`。测试0.091秒，明确报四类缺失；Slow注册正常。
- UE-MCP将四个资产重命名为`GC_Wizard_Combat_Burst`、`GC_Wizard_Combat_Windup`、`GC_Wizard_Skill_Impact`、`GC_Wizard_Skill_Shield`并编译保存。GameplayCueName读回均等于实际标签，限定旧路径的重定向检查无残留。没有改动地图、业务机制、飘字或震屏强度。
- 修复后新进程回归：同入口`-Action Test -TestFilter Wizard.Combat.Feedback`，退出0，`Saved/T01/20261102-013650-Test-206494/AutomationReport/index.json`，4 succeeded / 0 failed / 0 notRun。覆盖分发表、实际结算数字、反馈不修改业务及顿帧恢复；未重复运行无关测试。
- 观察者声音编辑态检查：SC_Eye的WavePlayer→Modulator连接、Dark/1__1_声源引用、音量及衰减均有效；SoundWave为48kHz双声道、48000采样。恢复WAV峰值31674、RMS7421（16位PCM），不是静音源。实际设备播放及听感仍由用户复验。
- 因涉及资产重命名及Cue扫描，执行`-Action Package`，退出0，`Saved/T01/20261102-013759-Package-291062/result.json`。随后检查发现成品缺少整个GAS/Cues目录：GameplayCueNotifyPaths不等于Cook包含规则，标签间接引用未带入这些蓝图。此包仅证明流程成功，不作为修复交付包。
- 第二项根因修复：通过UE-MCP向DefaultGame.ini追加`+DirectoriesToAlwaysCook=(Path="/Game/Wizard/GAS/Cues")`，保留已有Loot包含规则。为确认成品资源完整性重新打包；这是因发现明确遗漏而重跑，不是无变更重复打包。未启动任何游戏。
- 最终成品检查：`Saved/T01/20261102-014027-Package-306815/result.json`退出0，Cook汇总0错误0告警，Build/Stage/Package/Archive完成（本次源码未再修改，UBT目标up to date）。六个Cue蓝图均出现在Cook产物与IoStore输入清单，其中五个为本轮使用的处理器；清单副本`Saved/T01/20261102-014027-Package-306815/UAT/PakListIoStore_Wizard-Windows.txt`第685～690行。交付包为`Saved/Packages/20261102-014027-Package-306815/Windows/Wizard.exe`，旧包不包含完整修复。`git diff --check`通过。

Standards：遵循UE5.4资产注册契约，四个资产全部经UE-MCP修改；保留用户改动，无一次性反射工具、调试日志或运行效果验证入口。Spec：工程侧已修复缺失处理器；用户确认的飘字和震屏保留，声音及各技能可见效果仍待本次修复后复验，T12保持in-progress。

## 2026-11-02 第二轮材质、技能辨识与护盾修复

用户确认声音已恢复；截图显示冰箭弹丸/命中网格仍为棕色岩石，观察者有同类问题。另反馈普攻使用火球命中、闪现/护盾像雷击、闪现距离不足，护盾存续不可见。

- 冰箭保留指定Projectile2/Hit2项目副本，Mesh1的OverrideMaterials由原岩石替换为MI_IceCrystal；观察者对应两份系统使用MI_EnemyCrystal。材质分别覆盖蓝白/红紫色阶、粗糙度与边缘光，父材质M_CrystalBase为项目副本；原Free_Magic不修改。
- 普攻Impact/WorldImpact改为NS_ArcaneImpact（Hit3的短光点/闪光），不再用Hit1火球爆燃。闪现起终点改为NS_BlinkVortex符环/螺旋，移除冲击波引用；护盾开启/消散/承伤不再引用雷击冲击波，持续状态为蓝色透明球体与NS_ShieldOrbit光点。盾体补基础透明度以改善正面可见性。
- 按用户本轮距离反馈，DA_Blink.Range由600改为1000厘米，说明同步；近处鼠标目标、沿途阻挡、地面/胶囊落点逻辑不变，消耗/冷却不变。规格变更记录已同步。
- 护盾无盾体的代码原因：GE生成的Cue参数将技能定义保存在EffectContext，直接读取Parameters.SourceObject为空。统一改用GetSourceObject()，兼容直接参数和上下文来源；同时修复减速持续Cue同类问题。没有改变GE结算或护盾吸收规则。

工程证据与限制：

- Editor实际编译：`-Action BuildEditor`，业务修复`Saved/T01/20261102-015958-BuildEditor-04dedb`、最终测试源码`Saved/T01/20261102-020209-BuildEditor-f448ea`均退出0；本轮无反射声明变更，实际编译链接，不使用Live Coding。
- 新增CueEffectContext：由真实护盾GE Spec初始化参数，检查上下文来源、原生Cue创建持续盾体、附着目标、材质来源、重复激活不叠层及移除清理。初版测试通过BlueprintCosmetic包装调用，被未BeginPlay夹具拦截，`015929-Test-71ea00`与`020014-Test-6921c6`失败不能单独作为根因证明；改为原生处理边界后通过。不启动PIE或音画播放。
- 全量检查`Saved/T01/20261102-020014-Test-6921c6`中其余40项通过，新增用例因上述夹具原因失败；修正后执行`-Action Test -TestFilter Wizard.Combat.Feedback`，`Saved/T01/20261102-020233-Test-d7dee5`退出0，5 succeeded / 0 failed / 0 notRun。没有把前一失败报告记作全量通过。
- UE-MCP编译最终七份系统：冰箭弹丸/命中、观察者弹丸/命中、ArcaneImpact、BlinkVortex、ShieldOrbit，全部compiled=true、failedScriptCount=0。四份Mesh1覆盖材质读回正确。M_CrystalBase接通原有边缘光分支，M_CombatShield的Opacity连接基础透明度加轮廓分支。
- 工具清理：首次对两个新复制系统删除Emitter后，工具留下失效系统图，真实Niagara编译发现错误；改为完整复制并关闭不需要的Renderer，重新编译通过。两份失败临时副本NS_BlinkRift/NS_ShieldMotes在解除引用后通过MCP删除，没有留下坏图。未增加一次性C++工具或调试命令。
- 本轮集中一次Windows打包：`-Action Package`，`Saved/T01/20261102-020459-Package-fd986b/result.json`退出0，Editor/Game Build、Cook、Stage、Package、Archive完成；Cook汇总0错误0告警（UTS启动告警同前轮）。IoStore输入清单副本位于该目录UAT/PakListIoStore_Wizard-Windows.txt，确认两份晶体实例与三份新粒子入包。新版`Saved/Packages/20261102-020459-Package-fd986b/Windows/Wizard.exe`。`git diff --check`通过。

Standards：依据UE5.4 Cue上下文访问契约修复持续表现；资产经UE-MCP，原素材和地图布局保留。新增测试保护GE到表现的来源传递与盾体生命周期，避免整段护盾静默缺失或刷新残留，符合6.1；不为色值/粒子尺寸新增测试。Spec：本轮问题均已落实代码或配置修改，声音恢复为用户证据；冰晶材质观感、技能区别、1000厘米闪现手感、护盾实际可见性与群战遮挡仍待用户复验，T12保持in-progress。
