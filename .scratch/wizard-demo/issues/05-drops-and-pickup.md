# T05 图标掉落与自动拾取

- Status: done
- Blocked by: [T02](./02-basic-combat.md)、[T04](./04-inventory-and-items.md)
- Spec: R10、R15、R16、R17、R18、R19，正文见 [规格](../spec.md)
- Updated: 2026-11-01

## 目标与范围

将敌人死亡和物品事务接为金币、装备、药水、出售素材的配置掉落，完成带图标的地面物品、自动拾取及主动丢弃。

## 输入、输出与不变量

- 输入：一次性死亡事件、掉落配置、玩家接近/离开、丢弃数量。
- 输出：金币余额或背包变更、地面剩余数量与实际物品图标。
- 拾取与丢弃使用完整事务，失败不丢物品；金币不占格，死亡掉落只处理一次。

## 失败与边界

覆盖背包满、只能部分接收、重复接触、错误定义、缺失/失效图标降级、掉落生成失败及主动丢弃后仍处范围内。完全无法接收时地面对象保持有效；只在重叠开始时尝试一次，腾出容量后需离开重入。

## 验收条件

- [x] 四类掉落来自配置，地面对象引用实际定义并显示对应图标。
- [x] 范围内自动拾取；部分接收后数量准确留在地面；金币直接入账。
- [x] 丢弃数量准确扣减并生成对象，失败保持来源状态。
- [x] 丢弃者离开范围再进入后才恢复自动拾取。
- [x] 死亡/拾取重复事件、物品守恒及真实图标已有工程证据与用户完成确认；独立包实际加载留到T09/T10整体验收，不将Cook记为成品运行。

## 验证记录

2026-11-01用户明确确认“T05完成”，接受本票据功能交付，并授权提交、推送。工程实现、实际UHT/UBT、24项业务自动化、19项编辑态资产验证及Windows目标地图Cook已通过，证据见下文。历史记录保留各轮当时状态，最终状态以本节及末尾完成记录为准。
Standards：本轮重审0项确定缺陷，职责边界、GAS属性归属、事务守恒、重入与生命周期清理符合项目规范。Spec：R10、R15～R19及用户后续调整由工程证据与用户完成确认共同接受；独立包实际运行与完整循环仍由T09/T10验收，未声称本轮执行过Build/Stage/Package或独立游戏启动。

## 未决实现与学习

图标来源/许可、拾取半径、概率、数量与失败反馈在本任务配置并记录。讲解掉落对象、事务提交和范围事件的边界。

## 2026-10-31 开工

- 用户授权使用subagent实施；T02/T04均done。分工为地面对象/死亡掉落、背包丢弃/金币事务，主Agent负责整合审查、资产配置与工程验证。
- 实施依据为R10、R15～R19；金币不占格、部分接收剩余留地面、生成失败不扣来源、丢弃者离开再进入后才恢复拾取。沿用现有数量弹窗与PrepareDrop/CommitDrop，不提前实现装备、交易与落盘。
- 蓝图/资产仅通过UE-MCP配置，保留现有地图及用户改动。运行时效果按现行验证流程由用户验收，Agent不自主启动PIE或游戏截图；工程构建与必要业务自动化另行记录。

### 本轮实现与配置

- `Loot/WizardWorldPickup`持有地面剩余量，定义Icon为图标唯一来源；未激活对象可靠生成后，`InventoryComponent::Discard`才提交扣量并激活。UI沿用MVC与既有数量Modal，只转发业务请求。背包栈ID出包后退役/分离，地面Actor独占数量，回包建立新栈身份或合并已有栈。
- 金币由PlayerState持有int32余额与变更事件，精确入账检查非正数、溢出和回调重入，不占背包格；PlayerStatus新增可选GoldText与图标，绑定/换源/销毁沿用事件生命周期。
- 敌人既有一次性死亡门控内先生成掉落再广播击败；两类敌人数据引用`/Game/Wizard/Data/Loot/DA_EnemyDrops`。新挑战开始清理上轮所属掉落，结果展示期间保留；主动丢弃不归挑战清理。
- 并行审查发现并修复：仅轮询可能漏掉快速离开/重入，现改实际Pawn重叠范围与EndOverlap捕获离开，0.1秒扫描负责自动接收/容量重试；拒绝抽象/弃用拾取及Widget类，检查IconImage契约，并在有Slate的实际游戏扣量前创建图标，失败保留来源。
- UE-MCP创建并保存`BP_WorldPickup`、`WBP_PickupIcon`和`DA_EnemyDrops`；Config接入PickupClass。当前可调初值：拾取球半径160，图标40×40，散布85；金币2～6，矿石必掉1～4，生命/法力药水各30%掉1～2，法杖15%掉1。各条独立概率，不声称数值已试玩平衡。
- UE-MCP导入5张Game-icons.net透明PNG，四物品定义替换占位图标。原图、准确来源、Lorc/Delapouite署名及CC BY 3.0许可见`SourceAssets/Icons/GameIcons/ATTRIBUTION.md`；原像素未改，HUD金币乘金色。地图未修改。

### 工程验证（持续更新）

- `pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor`：`Saved/T01/20261031-200607-BuildEditor-e4ba4b`退出0，16项编译链接；审查修正后`Saved/T01/20261031-201214-BuildEditor-88631c`退出0，UHT3项、10项实际编译链接。均非Live Coding，非干净全量重编译。
- 第一版资产配置检查：UE-MCP `editor.validate_assets`显式检查InventoryConfig、EnemyDrops、PickupIcon、PlayerStatus、WorldPickup，5 valid/0 invalid/0 warning；BP_WorldPickup编译0错误0警告。最终源码修正后的校验另记。
- 首轮全量`-Action Test`在`Saved/T01/20261031-201303-Test-fde68a`退出3：新增Pickup临时World未设置GameInstance，真实WidgetComponent::GetOwnerPlayer断言失败；此前已完成测试结果不等于全量通过。修复测试夹具后重编复验，不放宽生产图标创建失败契约。
- 长期新增3项Pickup业务测试保护R17～R19的部分接收/满包守恒、金币精确入账与防复制、真实Discard失败不扣/旧请求拒绝/离开重入；后果是物品或金币丢失/复制，公开业务边界可重复检查，值得长期维护。扩展既有HUD生命周期测试检查金币刷新与换源解绑，不新增布局/像素断言。
- 学习材料：`.local-learning/lessons/2026-10-31-drops-and-pickup.md`与对应assessments；学习计划已追加，掌握度未测试。未提交/推送，保留用户未跟踪Config/DefaultDefaultEngine.ini。

### 最终工程结果与双轴结论

- 夹具补充最小GameInstance后重编：`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor`，`Saved/T01/20261031-201516-BuildEditor-ab84dd/result.json`，退出0，4项实际编译链接；本轮最后生产代码的UHT/UBT证据仍为前述88631c，未用Live Coding。
- 全量`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Test`：`Saved/T01/20261031-201603-Test-40957b/result.json`与`AutomationReport/index.json`，退出0，22成功/0失败/0未运行/0警告。包含三项Pickup及扩展PlayerStatus；使用独立Cmd/NullRHI临时世界，验证实际公开事务、Widget初始化与碰撞范围事件，不代表视觉、实际输入或成品包验收。
- 最终编辑器重开后通过UE-MCP重编PlayerStatus、PickupIcon、WorldPickup，均0错误0警告；明确校验InventoryConfig、EnemyDrops、两类敌人配置及3蓝图，共7 valid/0 invalid/0 warning；PickupIcon生成类/WidgetTree结构0 failure。读回掉落表、金币图标颜色与敌人引用一致。当前L_WizardClearing、非PIE、0脏包；未修改地图布局。
- Standards（引擎契约）：UE5.4本机WidgetComponent/UserWidget/WidgetBlueprintGeneratedClass接口、委托解绑与实际构建测试通过。框架与项目约定：Model事务、UI转发、PlayerState余额唯一来源、数据定义/资源与C++行为分离、MCP资产操作、无新增一次性装配入口；静态diff-check通过。7份校验与构建不替代Cook/成品加载。
- Spec：R10/R17～R19的失败不扣、部分接收、金币不占格、重入/重放拒绝、快速出入及自动重试有工程证据；R15配置覆盖四类与死亡只执行一次已接入并源码核对。R15实际击杀掉落、R16像素、暂停弹窗丢弃和独立包效果由用户验收，尚未执行，整票不转done。
- 未执行PIE、实际输入、运行截图、Cook或Windows打包；遵循现行用户负责运行效果、集中打包的验证约定。图标分辨率/大小、药瓶区分度、拾取半径与掉落初值待用户试玩反馈，不将配置初值称为最终美术或平衡确认。

### 用户运行验收入口

1. 在当前开发地图开始挑战，击败敌人；观察矿石/金币与随机药水、法杖图标，靠近后背包或左下金币余额变化。
2. Tab背包→物品详情→丢弃，确认指定数量；关闭背包仍站原地不得立即捡回，离开后返回应自动拾取，快速出入同样成立。
3. 将背包装满或只留下少量同类堆叠空间，靠近大堆掉落；检查能收多少只扣多少，剩余留地面，腾出容量后可继续接收；满包仍能接收金币。
4. 重复挑战确认旧轮所属地面掉落清理；实际图标/反馈与独立包验收结果另补，不以本轮工程结果代替。

## 2026-10-31 世界空间与发光提示

- 用户要求图标从屏幕空间改为世界空间，并添加光柱或发光粒子。规则已同步R16所在规格章节；本次只通过UE-MCP修改表现资产，C++拾取/丢弃事务未改。
- MCP已恢复：项目本地配置使用in-process模式避开daemon握手断开，原1.4.10及UE5.4兼容补丁保持。重启Codex后原生tools网关get_status返回editorConnected=true，当前编辑器PID32572。
- `BP_WorldPickup.PickupIcon.Space=World`、双面、无投影，IconSize为64×64（原40×40），相对高度46。核对当前角色蓝图CameraBoom=-60°，图标设置Pitch60/Yaw180匹配固定俯视相机；后续改动相机角度需同步配置。保留引擎Masked材质深度测试，读回Widget3DPassThrough.bDisableDepthTest=false。
- 新增`/Game/Wizard/VFX/NS_LootGlow`，复制现有Free_Spells Aura_Mana资源；BP新增Niagara组件LootGlow，Asset指向副本，自动激活、无AutoDestroy、相对高度8、缩放(0.3,0.3,0.65)。纯表现随Actor销毁，由组件生命周期管理；保留原包与护盾使用资源。副本采用原蓝色光环/星点与螺旋表现，属于用户允许的发光粒子方案，未宣称已制作独立直线光柱。
- 一次移除Smoke emitter的尝试导致本机UE5.4 Niagara系统图表残留节点，编译2脚本失败；已先解除新副本引用、通过MCP删除本轮失败副本，重新复制原资产并恢复引用。最终保留完整4个emitter，未将有错误副本交付。
- 最终MCP Niagara编译compiled=true、failedScriptCount=0、errors=[]；validate为4/4有效。BP编译0错误0警告，2份资产DataValidation为2 valid/0 invalid/0 warning。读回World、朝向、大小、特效引用/缩放/自动激活正确，保存0失败；星点及螺旋材质bDisableDepthTest=false。当前地图非PIE、0脏包且布局未改。
- Standards：表现参数与资源位于蓝图/资产，修改全程MCP，未新增业务代码、反射装配工具或测试。Spec：世界空间和发光配置已完成；遮挡、可读性、特效实际大小/持续性由用户新开PIE验收。本次未执行运行截图、自动化、UHT/UBT或Cook/打包，既有22项结果只证明此前业务版本，不作为本轮视觉验收。T05保持in-progress，未提交/推送。

## 2026-10-31 金色星点与中心旋转光晕

- 用户已自行调整NS_LootGlow并设无限循环，要求改为闪亮金色且中心持续旋转光晕。开工读回仅剩Sparks1/Sparks2两组，BP_WorldPickup有用户未保存修改；本轮在现状上局部修改并保存，未替换系统/恢复已删除emitter或重设循环、数量、运动、缩放。
- 通过MCP只更新系统ExposedParameters.ParameterData中已确认偏移的两个RGB：User.Color_Sparks1=(30,14.4,1.5)，User.Color_Sparks2=(20,10,0.8)，保留Alpha1/2与HDR峰值30/20。改后读回仅17个字节有变化，全部位于两颜色RGB，其他暴露参数保持。LootGlow组件OverrideParameters为空，不存在旧蓝色组件覆盖。既有星点生命周期闪烁保留。
- 新增项目自建程序材质`/Game/Wizard/VFX/M_LootHalo`，Unlit/Additive、双面、深度测试开启。UV径向光环+三段亮弧以Time旋转并轻微脉动，无外部纹理。公开材质参数GoldColor=(1,.48,.04)、GlowIntensity=12、RotationSpeed=.18圈/秒（约5.6秒一圈），可在材质实例中调整；材质图8节点全部可达。
- BP_WorldPickup新增LootHalo StaticMeshComponent，引用引擎Plane与M_LootHalo，位置(1,0,44.3)，匹配当前图标中心并略置其后；Pitch=-30适配固定俯视视角，缩放(1.2,1.2,1)，外圈直径约96单位。无碰撞、无Overlap、无导航影响、无阴影；材质动画不增加Actor Tick，随物品Actor一同销毁。用户已有LootGlow组件配置保持。
- 编辑态检查：Niagara编译compiled=true/failedScriptCount=0/errors=[]；BP编译0错误0警告；M_LootHalo重编译请求成功、图检查valid=true/issues=[]/8节点可达；3份资产DataValidation=3 valid/0 invalid/0 warning，内容保存0失败。未以材质图检查冒充运行像素或独立包Shader验收。
- Standards：资源和表现均经MCP配置，未改C++/事务/输入/循环业务，无新增工程测试或一次性装配入口。Spec：金色与旋转光晕配置已完成，实际亮度、尺寸、速度及中心位置由用户试玩验收；本轮未启动PIE/截图、未运行自动化/UHT/UBT/Cook。T05仍in-progress，未提交/推送。

## 2026-10-31 用户取消旋转光晕

- 用户反馈效果不理想，要求删除金色旋转光晕。通过UE-MCP移除BP_WorldPickup中的LootHalo组件，解除唯一材质引用后以force=false删除专用M_LootHalo资产；引擎Plane资源保留。
- 读回组件列表仅有LootGlow、PickupSphere、PickupIcon，确认LootHalo已移除。未修改NS_LootGlow金色星点、用户无限循环或现有组件变换；规格同步取消中心旋转光晕。
- Standards：全部资产操作经MCP，BP编译0错误0警告、DataValidation为1 valid/0 invalid/0 warning，保存0失败。Spec：删除及配置读回通过，未执行PIE/截图、构建、自动化或打包；实际效果由用户验收。T05保持in-progress，未提交/推送。

## 2026-10-31 参考图光柱与按物品类型区分

- 用户提供参考图，随后明确普通物品/金币仅底部柔光，装备额外有细金色竖直光柱，星点不保留。规格已同步；旋转光晕不恢复。
- MCP新增程序材质M_LootBeam、M_LootGroundGlow，均Unlit/Additive、双面、深度测试开启，无新增贴图；金色(1,.48,.04)、强度初值20/6，约6%～8%慢速明暗变化。光柱横向软边、两端渐隐；底部柔光为径向高斯亮斑，无旋转环。
- BP_WorldPickup新增LootBeam与LootGroundGlow：引擎Plane、无碰撞/Overlap/导航/阴影。光柱位置(0,0,180)、Pitch90、缩放(3.6,.6,1)，竖直高度约360、朝向当前固定俯视相机；落点柔光位置(0,0,2)、水平缩放.95。均附属PickupSphere，随Actor销毁。原LootGlow星点组件已通过MCP移除，用户改过的NS_LootGlow资源保留但掉落不再引用。
- 光柱默认隐藏并带EquipmentBeam组件标签；C++ SpawnPrepared在定义赋值后，仅Equipment类型开启带此标签的表现组件，金币/Consumable/Material保持隐藏。新增同类装备无需再写分支；参数与资源保持蓝图配置，UI/事务/拾取范围不改。
- 编辑态初检：两个材质图各7节点全可达、issues=[]，重编译请求成功；BP编译0错误0警告；3资产DataValidation为3 valid/0 invalid/0 warning。保存0失败，地图未改/非PIE。首次冷编译在编辑器尚未完全退出时被脚本存活保护拒绝（5635d5），未实际编译；确认退出后再执行，结果另记。
- 最终工程检查：`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor`，`Saved/T01/20261031-213233-BuildEditor-6f70bc/result.json`退出0，4项实际编译链接（非Live Coding，未新增反射）。`-Action Test -TestFilter Wizard.Pickup`，`Saved/T01/20261031-213418-Test-eadd07/result.json`退出0，3成功/0失败/0未运行。为低风险表现分类不新增实现镜像测试，已有公开拾取事务用例用于检查生成/拾取回归，不证明视觉分类或实际像素通过。
- 重开编辑器后再次校验3份资产均valid/0warning，并读回LootBeam默认隐藏及EquipmentBeam标签。Standards：C++决定类型、表现资产声明组件，资产改动全程MCP，构建/受影响既有测试/编辑态检查通过。Spec：一般物品/金币柔光、装备额外光柱已接入；亮度、比例、类型显示和参考图相似程度仍待用户实际运行验收。本轮未启动PIE或游戏效果验证，未Cook/打包，T05保持in-progress，未提交/推送。

## 2026-10-31 图标缩小与柔光对齐

- 用户反馈当前特效看着可以，要求缩小物品图标并调整到柔光中心；此反馈仅确认当前特效观感，不提升整票验收状态。
- 仅通过MCP修改BP_WorldPickup默认值：IconSize与PickupIcon.DrawSize从64调整为32×32；PickupIcon相对位置从(0,0,46)调整为(-9.2376,0,18)。当前CameraBoom为Pitch=-60/Yaw0，柔光中心相对位置(0,0,2)；将高差16沿朝向相机方向补偿X=-16/tan(60°)，减小浮空投影偏移，保留图标朝向和深度遮挡。固定方向补偿用于当前相机，屏幕边缘透视差异仍需用户试玩检查。
- Standards：BP编译0错误0警告，DataValidation=1 valid/0 invalid/0 warning，尺寸与位置已保存并读回；纯表现调参未改C++，不新增测试或重跑构建。Spec：尺寸/对齐配置完成，新生成掉落的实际视觉由用户验收；未启动或操控PIE、未截图/打包。柔光和装备光柱参数未改，T05仍in-progress，未提交/推送。

## 2026-10-31 T05代码审查

- 范围：以`c2cbf1fa232b0aa8ccdaff2249d469349f042b03`（HEAD）为基线，审查工作区T05源码差异，包含未跟踪的`Source/Wizard/Loot/`和`WizardPickupTests.cpp`；未修改业务源码或资产。
- Standards：独立规范轴审查未发现需要修正的确定缺陷或有充分依据的代码异味。核对编码规范第2、4、5、6.1节的职责边界、事务、引用、委托/定时器清理及长期测试依据；通过本机UE5.4源码确认WidgetTree运行时可用和EndOverlap通知时序。
- Spec：独立规格轴审查未发现可确认的实现缺陷或范围扩展。R10、R15～R19的配置掉落、一次死亡门控、部分接收、金币独立入账、失败保留来源与离开重入路径源码闭合。
- 证据：复核`Saved/T01/20261031-213233-BuildEditor-6f70bc/result.json`（构建退出0）及`Saved/T01/20261031-213418-Test-eadd07/result.json`（3项Pickup成功、0失败、退出0）；本轮未重新执行构建或自动化。`git diff --check`退出0。通过UE-MCP只读读回EnemyDrops的四类掉落、InventoryConfig同一组定义及拾取类、PickupSphere碰撞响应、PickupIcon世界空间和LootBeam默认隐藏配置。
- 覆盖边界：现有Pickup自动化主要检查公开拾取/丢弃事务，并不覆盖敌人实际击杀到掉落的完整链路；实际图标、类型表现、玩家输入与独立包资源加载继续待用户验收。本轮未启动/操控PIE、未截图、Cook或打包。两轴各0项代码发现不等于整票验收完成，Status保持in-progress。

## 2026-10-31 移除临时初始背包物品

- 用户确认初始背包为空，删除临时初始物品配置。移除`FWizardInitialItem`、Config/Component的`InitialItems`、`InitializeInventory`及其发放标记；角色初始化不再发放背包物品，快照恢复继续走既有事务。
- 先通过UE-MCP清空并保存`DA_InventoryConfig.InitialItems`，读回为空；随后删除源码中的字段，避免资产残留样例物品数据。该资产现有Definitions与PickupClass仍被运行代码引用，本轮保留；此前讨论的目录/拾取模块/统一拾取类重构尚未实施，不把保留资产误称为已删除。
- 删除初始批量发放专用断言，药水夹具改用公开AddItems准备物品；保留非法定义拒绝接收检查，并在既有玩家/GAS测试中验证新玩家与重复角色绑定均为空背包。长期保留理由：保护用户确认的空背包规则与真实药水事务，避免角色绑定意外发放物品；不保留已删除临时发放功能的测试。
- 编辑态资源读回后当前地图为L_WizardClearing、非PIE、0脏包；通过MCP安全关闭干净编辑器后冷编译，完成后重新打开项目，未操控PIE或做运行效果验收。
- `pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor`：`Saved/T01/20261031-221726-BuildEditor-b393e6/result.json`退出0，UHT输出3份、实际12项构建动作；非Live Coding，非干净全量重编译。
- `pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Test`：`Saved/T01/20261031-221826-Test-bcbea8/result.json`与`AutomationReport/index.json`，退出0，22成功/0失败/0未运行，含调整后的背包定义检查、空背包与药水事务及3项拾取测试。源码/Config/Tools检索已无InitialItems、FWizardInitialItem或InitializeInventory调用。
- 重开编辑器后UE-MCP读回DA_InventoryConfig只剩Definitions、PickupClass与NativeClass，InitialItems已不存在；显式DataValidation为1 valid/0 invalid/0 warning。
- Standards：项目约定层按规范2.1清除临时功能与专用断言，无新增装配入口；引擎契约层UHT/UBT与业务自动化通过，git diff --check退出0。Spec：新玩家20格为空及重复角色绑定不发放物品有自动化证据，R09/R10/R14及拾取事务相关测试通过；本轮未执行实际输入、PIE效果、Cook或成品包验收，T05保持in-progress。

## 2026-10-31 删除背包目录并改为玩家重叠事件拾取

- 用户要求删除DA_InventoryConfig、迁移掉落检测、解除图标业务限制；随后明确禁止计时器，只在重叠开始时尝试一次。最终规则已同步规格第6节，本节替代历史记录中的背包目录登记、每掉落定时扫描及原地容量重试方案。
- 删除UWizardInventoryConfig、Catalog、按ID接收接口与UI查目录接口。背包AddItems直接接收定义，运行条目强持有定义引用，快照序列化仍只保留稳定DefinitionId；恢复时通过AssetManager自动扫描`/Game/Wizard/Data/Items`解析`WizardItem:ItemId`，无需手工总目录。重复实例ID、非法数量、同一ID对应不同定义仍拒绝。
- 新增PlayerState持有的WizardPickupComponent，仅订阅当前Avatar的Actor级BeginOverlap/EndOverlap与EndPlay。BeginOverlap尝试一次，EndOverlap只解除丢弃门控；无定时器、Tick、轮询、世界玩家枚举或容量变化重试。换Avatar解除旧订阅，同Avatar重复绑定不补发拾取；角色绑定时不主动扫描已有重叠。
- WorldPickup仅持有内容、碰撞形状、表现与剩余量，TransferTo锁定同步交接并扣实际接收量，归零销毁；不引用玩家/GAS/背包。主动丢弃仍在可靠生成后扣来源，再由玩家模块登记离开门控并启用碰撞。
- 移除掉落表PickupClass，唯一类配置移至Project Settings的Wizard Loot（`DefaultGame.ini`），共用现有BP_WorldPickup。先确认资产引用者为0，再通过MCP解除原生CDO中的Config引用、以force=false删除DA_InventoryConfig；旧配置类、字符串路径与运行目录接口均已移除。地图和现有图标/柔光/光柱参数保持。
- 图标缺失或加载失败由共享ResolveIcon返回引擎DefaultTexture；背包格、拖拽预览、地面图标及金币共用降级结果。掉落表和地面生成只校验业务数据，Widget创建/布局失败不再销毁有效掉落或拒绝丢弃；正式图标仍按R16单独验收。
- 资产与配置全部经UE-MCP：保存4份定义以更新PrimaryAsset元数据、保存EnemyDrops以移除旧字段；扫描规则AlwaysCook，`/Game/Wizard/Loot`纳入Cook目录。显式7份资产DataValidation为7 valid/0 invalid/0 warning。重开后读回EnemyDrops已无PickupClass，当前L_WizardClearing、非PIE、0脏包。
- 首次冷编译请求`Saved/T01/20261031-223409-BuildEditor-17b0b7`因编辑器关闭尚未完成被脚本保护拒绝，未执行UBT。确认退出后`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor`：`Saved/T01/20261031-223424-BuildEditor-7e82d4/result.json`退出0，UHT12份输出、22项实际构建动作，非Live Coding、非干净全量重编译。
- 全量`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Test`：`Saved/T01/20261031-223815-Test-eba4d9/result.json`与`AutomationReport/index.json`，退出0，24成功/0失败/0未运行。拾取用真实碰撞Begin/End事件验证初始重叠接收、部分/满包守恒、腾出容量不自动重试、快速重入、金币溢出/重入、丢弃失败不扣、无图标、换角解绑。新增DefinitionIdRestore通过SaveGame标志的真实结构序列化往返验证运行指针不落盘、自动ID解析与未知ID失败不丢现有库存；不宣称T08文件保存功能已完成。
- 长期测试取舍：调整原有事务/菜单/拾取测试以跨公开接口和真实重叠验证规则，撤除依赖目录登记与定时扫描的断言；新增两个用例保护无图标/换角拾取守恒和定义ID序列化恢复，后果为物品丢失、复制或读档无法恢复，值得保留。没有布局/像素断言、一次性装配入口或测试专用运行命令。
- 目标地图Cook：`UnrealEditor-Cmd.exe Wizard.uproject -run=Cook -TargetPlatform=Windows -Map=/Game/Wizard/Maps/L_WizardClearing -unattended -nop4 -NullRHI -UTF8Output -stdout -DisablePlugins=UE_MCP_Bridge -OutputDir=<本轮Cooked> -abslog=<本轮日志>`；证据`Saved/T05/20261031-224050-Cook/result.json`及`cook-engine.log`，退出0，895包、0错误/0警告。独立输出目录包含BP_WorldPickup、WBP_PickupIcon、4份定义、EnemyDrops、物品/金币图标与引擎DefaultTexture，无DA_InventoryConfig；不等同Build/Stage/Package或成品运行验收。
- Standards：引擎契约层UHT/UBT、结构序列化、重叠事件与解绑通过实际测试；项目约定层职责分离、无轮询/定时器、直接定义引用、数量事务、配置/资产MCP操作及长期测试依据符合规范2/4/5/6.1节，git diff --check通过。Spec：R10/R15/R17～R19及本次用户规则的业务证据通过；R16的真实占位图标/遮挡/光柱效果、暂停Modal物理操作及独立包运行仍由用户验收，未启动PIE或游戏截图，T05保持in-progress。未提交/推送。


## 2026-11-01 两类怪物独立素材掉落

- 用户指定史莱姆掉落史莱姆粘液/史莱姆眼睛，观察者掉落观察者触手/观察者眼睛，允许图标生成及概率数量自拟。PBR模型与动画配置详见T09本日记录。本轮没有新增C++、测试源码、业务分支或轮询。
- UE-MCP新建四份WizardItemDefinition：DA_SlimeMucus、DA_SlimeEye、DA_BeholderTentacle、DA_BeholderEye，稳定ItemId分别与英文名一致；中文名按用户要求，Type=Material，MaxStack=50，PotionResource=None，数量/定义有效。自动纳入已有WizardItem扫描与AlwaysCook目录，无新增背包目录。出售接口与价格属于T07，本轮只配置材料定义，不提前实现交易。
- 通过UE-MCP分别创建DA_SlimeDrops、DA_BeholderDrops，并设置BP_Slime/BP_Beholder的DropConfig。旧共享DA_EnemyDrops解除引用后force=false删除，原矿石定义保留，两个怪物不再掉矿石。各行独立概率判定，可同时掉落。

| 怪物 | 掉落 | 概率 | 数量 |
| --- | --- | --- | --- |
| 史莱姆 | 金币 | 100% | 2～4 |
| 史莱姆 | 史莱姆粘液 | 100% | 1～3 |
| 史莱姆 | 史莱姆眼睛 | 25% | 1 |
| 观察者 | 金币 | 100% | 4～8 |
| 观察者 | 观察者触手 | 80% | 1～2 |
| 观察者 | 观察者眼睛 | 35% | 1 |
| 两种怪物 | 生命药水、法力药水 | 每项20% | 每项1 |
| 史莱姆/观察者 | 学徒法杖 | 5%/10% | 1 |

- 保留药水/装备掉落满足R15，专属材料替换旧矿石，散布半径仍85。上述仅试玩初值，未声称已平衡。
- 原项目未发现对应的四种素材图标。按用户授权使用imagegen技能与内置image_gen，四次独立生成白色透明背景图标，原始PNG均1254×1254 RGBA、alpha范围0～255。未作像素编辑。原图保存SourceAssets/Icons/GeneratedMonsterMaterials/，完整提示词与方式见同目录GENERATION.md；经UE-MCP导入T_SlimeMucus/T_SlimeEye/T_BeholderTentacle/T_BeholderEye并接入各物品Icon。设置EditorIcon压缩、UI LOD、sRGB、NeverStream，保留Alpha。来源目录已纳入工程，不只存在Codex缓存。
- 编辑态验证：editor.validate_assets明确选择19份相关资产（两个BP、两个ABP、两个BS、两个AM、两掉落表、四定义、四纹理及地图），19/19 valid、0警告。read_properties读回掉落行、概率数量与各定义；删除旧表前未强制覆盖其他引用。工程验证见后续条目。

- 业务回归：`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Test`，证据Saved/T01/20261101-001055-Test-030560/result.json及AutomationReport/index.json，退出0；24成功/0失败/0漏跑，测试事件0警告/0错误。使用现有编译产物验证本轮资产变化，没有新增测试源码。

- Windows目标地图Cook：UnrealEditor-Cmd.exe Wizard.uproject -run=Cook -TargetPlatform=Windows -Map=/Game/Wizard/Maps/L_WizardClearing -unattended -nop4 -NullRHI -UTF8Output -stdout -DisablePlugins=UE_MCP_Bridge -OutputDir=<本轮目录>/Cooked -abslog=<本轮目录>/cook-engine.log。证据Saved/T09/20261101-001248-PBR-MaterialDrops-Cook/result.json、cook-engine.log；退出0，900 packages，0 errors/0 warnings。输出包含两份新掉落表、四定义、四图标及PBR模型/配套动画/材质，不含旧共享表及Polyart模型目录。没有执行Build/Stage/Package或游戏启动。
- Standards：本轮无C++新增或业务分支；资产写入全部经UE-MCP，19项DataValidation与24项现有回归通过，动画骨架/片段引用一致，图标源文件及生成提示词进入工程，git diff --check通过。未引入临时自动化入口或永久配置镜像测试。
- Spec：用户指定的两种PBR怪物及四种专属素材已配置，金币/药水/装备基础掉落仍保留（R15），新图标引用与Cook有效（R16），拾取事件行为沿用既有实现。编辑器已重新打开；实际PBR材质外观、动画衔接、32像素图标辨识度、实际击杀/拾取与独立包运行待用户验收，相关票据保持in-progress。未提交或推送。

## 2026-11-01 用户确认完成与交付

- 用户明确回复“T05完成，提交并推送git”，本票据转done；T02/T04前置均已完成。用户完成确认作为本票据功能交付依据，未添加未经用户报告的具体试玩步骤或独立包运行结果。
- 最终功能包括：单一地面掉落蓝图、玩家BeginOverlap单次接收、部分容量留余量、主动丢弃离开重入、金币独立入账、图标缺失降级、空初始背包及删除DA_InventoryConfig；两种PBR怪物各用独立掉落表，四种专属素材与图标已接入。
- 同轮关联交付包含EnemyConfig删除与怪物蓝图默认值迁移、PBR模型及匹配动画、现有地图靶子表现适配；T09完整游戏循环仍保持in-progress。
- Standards：2026-11-01对当前全部未提交源码、配置与相关资源重新审查，0项可确认缺陷。核对事务守恒、回调重入、委托/任务清理、GAS唯一属性、C++/蓝图边界，无临时装配入口残留。
- Spec：R09～R19及本票后续用户调整复核无新增缺陷；MCP读回两怪物独立参数、PBR模型/骨架/ABP/BS/AM依赖、各自掉落表和四种素材定义均正确，8份动画/怪物资产共40项依赖无Polyart残留。
- 工程证据沿用最终源码BuildEditor（Saved/T01/20261031-235041-BuildEditor-6e6d47，退出0）、24项Test（Saved/T01/20261101-001055-Test-030560，24/0/0）、19项DataValidation与Windows地图Cook（Saved/T09/20261101-001248-PBR-MaterialDrops-Cook，900 packages、0错误/0警告）。审查后仅更新交付文档，不重复构建、测试或Cook。
- 独立包实际加载、完整经济/装备/存档循环仍在T09/T10验收；本票完成确认不代表R29或整个Demo已完成。学习资料保留在.local-learning，未纳入Git，用户掌握程度未测试。
- 提交前检查：编辑器content/map脏包均0；Git暂存范围包含本票实现、关联敌人重构、配置/资源、必要长期业务测试、来源/许可/生成提示词与验证摘要。用户独立文件Config/DefaultDefaultEngine.ini保留未跟踪；本机Saved原始日志不入库。
