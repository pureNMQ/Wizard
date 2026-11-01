# T07 商店与出售素材

- Status: done
- Blocked by: [T04](./04-inventory-and-items.md)、[T06](./06-equipment-and-bonuses.md)
- Spec: R03、R04、R13、R23、R24、R25，正文见 [规格](../spec.md)
- Updated: 2026-11-01

## 目标与范围

在营地商人NPC自身接入商店交互，复用物品定义和事务完成购买、出售与素材堆叠出售，首版库存无限。商人使用BattleWizardPBR的WizardSM模型与PBRMaskTintMat3Inst材质。

## 输入、输出与不变量

- 输入：商品/定义、买卖方向、数量、金币与背包状态。
- 输出：可区分的交易结果及一次提交后的余额和物品状态。
- 金币唯一货币，价格/目录来自配置，库存规则按规格；装备中的物品先卸下再售。

## 失败与边界

验证金币不足、容量不足、错误定义/价格/数量、数量乘价范围溢出、不可交易物品及重复请求。任意失败均不扣金币、不改变物品。

## 验收条件

- [x] 靠近营地商人NPC按E打开真实 CommonUI 商店，四类装备和两类药水均可购买；模型及材质符合用户指定。
- [x] 购买、出售通过数量选择与明确按钮提交，价格/目录为配置。
- [x] 背包内可交易装备、药水和素材可售，装备中物品不可直接售。
- [x] 暂停中交易/返回可用，界面刷新读实际业务状态。
- [x] 失败交易、数量/金币一致性与新增商品/素材配置有验证。

## 验证记录

2026-11-01用户明确确认“T07完成”，接受本票据功能交付并授权提交、推送。工程实现、UHT/UBT、34项自动化及相关编辑态配置检查已通过；审查后的P3修复有实际增量构建与5项交易专项回归通过证据，详见下文。历史记录保留各轮当时状态，最终状态以本节及末尾完成记录为准。交易触发持久化的集成验收在T08，Cook/独立包留待T09/T10阶段整合。
Standards：未发现未解决的确认源码缺陷，P3重复属性名称映射已修复。Spec：R03/R04/R13/R23～R25由工程证据与用户完成确认共同接受；不将用户完成确认写成Agent执行过实际输入或成品验收。

## 未决实现与学习

商品初值、图标和交互键位继续按规格标识处理。讲解同一物品定义如何复用到目录、掉落与出售。

## 2026-11-01 开工与界面确认

- T04、T06均done，前置满足；用户确认双栏SVG草图及商品分类，并授权开始实施、使用subagent。
- 左侧商品列表显示图标、名称、单价；顶部“全部 / 装备 / 药水”分类仅筛选商品，默认全部。选中商品下方固定详情与购买入口。
- 右侧20格背包，不显示属性与装备栏；格子保持正方形、无名称、堆叠数量右下角，可交互详情悬浮窗提供出售入口。余额与关闭入口位于主面板内。
- 买卖按钮打开既有Modal层的数量选择，整数输入框和滑动条双向同步、明确确认/取消；CommonUI复用Menu/Modal及集中暂停，商店复用场景交互E键。
- 商品价格作为可调试玩初值；出售素材不列为可购买类别。失败交易不提交，金币与物品同次变更。完整保存/重启往返留给T08。
- 本轮分工：交易模型与测试、商店MVC、场景入口与会话测试；主Agent负责集成、UE-MCP资产配置、工程验证和证据。运行效果由用户验收，不自行启动PIE或操控游戏；未授权提交/推送。
- 保留已有T01票据、用户地图、本机配置与卡顿调查文档改动。Standards/Spec待实现后分别记录，不提前标完成。

2026-10-29 用户要求交互提炼复用，T03已提供事件驱动交互模块。T07在商店Actor中配置UWizardInteractionTarget并绑定原生可用条件/打开CommonUI动作，复用现有E键和HUD事件，不增加场景定时扫描。接入及动态可用性通知约定见[交互说明](../../../docs/INTERACTION.md)。当前仅提供接缝，未据此认定商店或交易已实现。

## 2026-11-01 实现与工程交付

### 实现及配置

- `UWizardShopCatalog`保存商品定义、正整数买价和无限库存规则；物品定义新增`bTradable/SellPrice`，十五份既有定义已配置，目录包含八件装备和两类药水。数值为试玩初值，详见[商店说明](../../../docs/TRADING.md)。没有初始金币/物品发放。
- Inventory新增Buy/Sell、只读购买上限与失败原因查询；先校验完整结果，以int64检查乘价，再同时更新背包和PlayerState唯一余额，之后广播。仅向物品组件开放受限friend提交边界，通知期间两边拒绝写入重入；失败不改状态，装备区保持不变。
- ShopMenuController冻结会话、版本、来源实例、定义和报价；确认重验场景权限及目录身份，改选不改冻结来源、重放/关闭重开拒绝旧请求。MVC视图复用既有InventoryTile、Popover与QuantityModal；普通背包的拆分/丢弃仍走原路径，不复制整数输入/slider实现。取消、失活、销毁清理回调和事件。
- UE-MCP创建并保存`WBP_ShopMenu`、`WBP_ShopProductRow`、`WBP_ShopPopover`、`DA_CampShop`、`BP_WizardShop`。双栏、三分类、固定商品详情、20格背包、出售悬浮窗、金币图标与关闭入口均已接线；列表可滚动，复用既有Menu/Modal，不显示属性和装备栏。分类切换后焦点移交有效商品行，默认及返回焦点为可用关闭按钮。
- 当前手工地图仅新增`Shop_CampMerchant`适配Actor，引用既有`Camp_MerchantHouse`；营地盒体沿用炼金台中心(-2200,0,100)与范围(1500,1500,500)。商店模型体积较大，交互半径设550cm并读回确认；原房屋与两份棚顶的变换前后读回一致，没有重跑布景脚本。实际接近路线与遮挡仍待用户试玩。
- 场景入口与每次提交均重验真实InteractionTarget权限（存活、非挑战、营地、距离、遮挡、目标启停、目录/界面配置），复用E输入和HUD提示，无定时扫描。

### 工程验证

- 首轮BuildEditor因`TObjectPtr`数组使用`auto*`范围遍历推断失败，已改显式类型；失败证据`Saved/T01/20261101-184050-BuildEditor-15429d/`，退出6，不计通过。中间构建`20261101-184210-BuildEditor-d602c3/`通过，不代替最终焦点修正版本。
- 首轮全量Test在新增目录测试中直接`Products.Add(Products[0])`触发UE TArray自引用断言（测试构造错误），已先复制独立值再Add，并检查其他插入；失败证据`Saved/T01/20261101-184246-Test-e8f4ca/automation-engine.log`，退出3。该失败没有记为功能通过，也没有通过删除关键断言规避。
- 最终`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor`：真实UHT/UBT增量编译、链接成功，退出0，`Saved/T01/20261101-184844-BuildEditor-fba028/`；非Live Coding，非干净全量重编译。
- 同版本统一`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Test`：34 succeeded / 0 failed / 0 notRun，退出0，`Saved/T01/20261101-184958-Test-90026a/AutomationReport/index.json`。包含新增`Wizard.Trading.AtomicTransactions`、`ConfigurationAndNotificationReentry`、`FailureBoundaries`、`InteractionPermissions`、`MenuSession`五项，以及既有装备/GAS、背包、掉落、输入/交互等工程回归。
- 长期测试取舍：五项新增测试保护R23～R25与交易守恒、失败不提交、金额溢出、装备不可直售、通知重入、旧会话/报价/来源拒绝及真实场景权限；回归可能造成丢货/复制/扣错金币，公开接口验证稳定、维护收益明确。未保留一次性装配测试或运行效果验证入口。
- 编辑态UE-MCP：十五份物品定义15 valid/0 invalid/0 warning；最终商店目录、新增三份Widget和商店BP及受影响既有背包四份Widget共9 valid/0 invalid/0 warning。Menu绑定11 satisfied，Row含可选项5 satisfied，Popover含SellButton5 satisfied，均0 missing/0 wrongType；Menu结构pass/0 failures/0 unreachable。
- 配置读回核对十项商品定义及价格、Menu三项子类引用、商店实例目录/界面/550cm半径、标题“商店”、分类“全部/装备/药水”、纯数字余额与T_Gold纹理、112高详情区和可滚动商品列表。Image颜色的便利参数需逗号RGBA而非UE结构文本，发现图标透明后已修正并读回；TextBlock金币及价格颜色为正确不透明金色。Widget编译保存与数据检查不能代替实际布局验收。
- `git diff --check`通过（仅既有换行提示）；本轮无临时C++桥接/调试入口、无额外运行命令。学习材料：`.local-learning/lessons/2026-11-01-t07-trading.md`（已确认Git忽略，掌握程度未评估）。

### 双轴结论与待用户验收

- Standards：依据UE5.4契约检查反射、对象引用与生命周期，依据CommonUI/Enhanced Input框架实践检查复用入口和层级，依据项目约定检查完整事务、MVC、委托/回调清理、MCP资产修改及长期测试保留。独立subagent复核与主Agent集成检查未发现未解决的确认源码缺陷；构建/34项自动化/编辑态配置通过。
- Spec：R23～R25商品目录、通用素材出售、买卖原子性及失败边界已有工程证据；R03/R04/R13真实暂停输入、数量输入/slider、Esc、焦点、悬浮窗可达性及页面视觉均待用户运行验收。不以自动化和配置读回替代用户操作，T07保持in-progress。
- 用户试玩：靠近营地商人NPC按E（入口按下文用户纠正已从房屋迁移）；检查三分类/选中详情，购买与出售数量选择、确认/取消、余额/数量同步；检查金币不足、满包、装备先卸下，Esc先关数量弹窗再关商店，关闭后不残留连射/移动。金币和物品通过实际战斗获得，不恢复临时发放。
- 未执行PIE/游戏操控、Cook或Windows打包；成品软引用与完整存档往返留待T08/阶段整合。未提交/推送，保留用户已有T01、本机配置、卡顿调查文档与地图改动；本轮地图保存包含在用户既有布局上新增的商店适配。

## 2026-11-01 用户明确商人NPC入口

- 用户纠正此前未明确的场景要求：交互应在商人NPC自身，指定BattleWizardPBR及PBRMaskTintMat3Inst；本段取代上文房屋目标与550cm半径方案，规格及商店/交互说明已同步。
- `AWizardShopActor`删除外部`ShopActor`引用，`InitializeTarget(this,...)`；范围、遮挡、提示与可用条件跟随NPC自身。更新既有InteractionPermissions测试，保留营地/存活/挑战/距离/遮挡/配置失败保护，并验证NPC移动后目标随自身离开与返回。不新增长期测试用例或一次性工具。
- UE-MCP在既有`BP_WizardShop`中配置MerchantMesh：`/Game/BattleWizardPBR/Meshes/WizardSM`、材质槽0覆写`/Game/BattleWizardPBR/Materials/PBRMaskTintMat3Inst`，不修改共享原模型/材质或玩家外观；新增`ABP_Merchant`，蓝图状态机MerchantIdle的Idle状态使用Idle01Anim，已连接到最终Pose。
- 商人Actor位于(-1650,700,90)，Yaw=-90；网格局部Z=-90、Yaw=-90，胶囊中心为Actor位置、半高90/半径34，交互半径250cm。下探命中Ground_Walkable地面Z=0；导航投影命中可行走区域；胶囊与商人房屋几何包围盒不重叠，分离约129cm。房屋与棚顶未移动。商人营地盒体随自身，仍为(1500,1500,500)，每次交易同时检查非挑战及实际250cm范围。
- 配置读回发现已放置SCS实例未继承模板的新材质/动画/变换；已通过MCP同步该实例并保存。最终场景MerchantMesh读回材质PBRMaskTintMat3Inst、AnimClass=ABP_Merchant_C、局部Z=-90/Yaw=-90；身体胶囊BlockAll/QueryOnly。不能仅以蓝图模板读回代替关卡实例配置。
- `BuildEditor`真实UHT/UBT增量编译链接通过，退出0，`Saved/T01/20261101-191235-BuildEditor-abd9ce/`；`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Test -TestFilter Wizard.Trading`为5 succeeded/0 failed/0 notRun，退出0，`Saved/T01/20261101-191329-Test-12a71f/AutomationReport/index.json`。本轮未改交易模型，不重复其他系统或Cook/打包。
- 商人BP和ABP均编译通过、0 warning，连同指定材质3份资产DataValidation为3 valid/0 invalid/0 warning。ABP根图读回状态机Pose已连接输出，状态资产为Idle01Anim；实际待机播放及E交互仍待用户运行验收。
- 编辑态截图仅作为配置排查辅助：最初截图揭示旧实例材质/悬空问题，修正后`Saved/T07/merchant-editor-final.png`可见指定配色；SceneCapture明暗异常，不将其当正式灯光/运行表现验收，也不据此修改既有场景灯光。没有启动或操控PIE/游戏。
- Standards：删除房屋业务引用，NPC蓝图负责资源/动画，C++继续负责交互条件与交易入口；相关构建、5项交易测试、资产编译/读回通过。Spec：NPC与指定模型/材质已配置，实际NPC触达、待机、提示、买卖界面仍待用户验收；T07继续in-progress。
- `git diff --check`通过；无临时工具，未提交/推送，用户已有改动保持。

## 2026-11-01 用户截图后的商店布局修正

- 用户截图`codex-clipboard-c88ed6cb-6ebf-4e51-ad66-f32a45f338c5.png`显示：默认浅灰商品按钮覆盖行选中底色，按钮内容整体居中导致名称与价格挤在一起；购买SizeBox在VerticalBox的Fill对齐下仍拉满整列，详情与按钮贴合，主面板过大且下方留白过多。截图作为运行问题证据，不能沿用先前编辑态检查声称视觉通过。
- 本轮仅通过UE-MCP修正商店Widget资产，不改C++或交易规则。主面板缩为1040×652并保持居中，内部Canvas明确商品、分类、滚动列表、详情、购买按钮与背包区的位置；上下/列间分隔、标题字号与Regular正文统一层级，不添加副标题或操作说明。
- 商品行34高，26×26图标在左、名称Fill占剩余宽度、价格靠右；ButtonSlot明确HAlign_Fill，去掉正常态灰色刷子，让C++已有SelectionBorder高亮显现，hover/press使用低强度底色。分类/关闭/出售采用深色样式，购买采用金色强调；购买固定148×40，独立位于详情下方右侧，不再被纵向容器拉宽。详情放在106高可滚动区域，长描述不会覆盖按钮。
- 复制既有Tile布局为商店专用`WBP_ShopInventoryTile`（同一C++基类），80×80尺寸，仍正方形、无名称、数量右下角；Menu.TileClass已读回指向新类，普通背包Tile保持96×96不变。四行五列共20格保持；商品列表272高，按34高行显示八行，可滚动至药水。
- 编辑态验证：Menu/Row/Popover/ShopInventoryTile四份Widget编译保存，4 compiled/0 failed/0 warning；DataValidation为4 valid/0 invalid/0 warning。Menu绑定11 satisfied/0 missing/0 wrongType，结构pass/0 failures/0 unreachable；读回购买Canvas偏移/148×40尺寸、商店Tile80×80、Row ButtonSlot Fill与名称SizeRule=Fill，以及主面板尺寸/颜色均正确。0个未保存内容/地图包。
- Standards：遵守纯UI编辑态检查分工、MCP资产操作、标题与文案约束，无业务或临时工具变更，`git diff --check`通过。Spec：既定双栏/三分类/20格及交易入口保留；本轮实际视觉、悬浮窗和点击体验仍待用户重新运行验收，T07保持in-progress。
- 未启动或操控PIE，未新增运行检查命令或截图；未重跑C++构建、自动化、Cook或打包，理由为纯布局/样式改动。未提交/推送。

## 2026-11-01 T07 代码审查

- 范围：以HEAD `ff72e3da8372b36c68f898b85cd5e3a3cfdeff3e`为基线，审查T07未提交的交易、商店MVC、背包/金币边界、UIRoot接入及五项交易测试；包含未跟踪源码。未将T01、本机配置和卡顿调查改动纳入T07审查。
- Standards：主审与独立规范审查未发现可确认的功能或引擎契约缺陷。非阻塞P3建议（判断性Duplicated Code）：`WizardShopWidgets.cpp:108-112`重复`WizardInventoryWidgets.cpp:30-39`中的五种属性中文名称映射，可共用显示格式化函数，避免以后改名遗漏；当前名称一致，不影响交易验收。
- Spec：独立规格审查与主审未发现可确认的源码缺陷；核对R23～R25数据目录、原子买卖、失败不提交、R24装备拒售，以及冻结来源/报价/会话和重复请求。R03/R04/R13复用既有Menu/Modal与数量控件的源码路径已检查，不能据此认定实际输入验收完成。
- 证据：本次`git diff --check`无空白错误（有LF/CRLF提示）；读回既有`Saved/T01/20261101-184958-Test-90026a/AutomationReport/index.json`为34 succeeded/0 failed/0 notRun，入口调整后的`Saved/T01/20261101-191329-Test-12a71f/AutomationReport/index.json`为5 succeeded/0 failed/0 notRun。本次未改游戏源码，未重复构建或执行测试；既有报告属于前次验证证据。
- 边界：本次为代码审查，未重新读回二进制Widget/地图资产，未启动或操控PIE，未执行Cook/独立包。NPC触达、暂停输入隔离、焦点、悬浮窗与数量弹窗、实际布局仍待用户运行验收；Status保持in-progress。仅写回审查记录，未修改实现、提交或推送。

## 2026-11-01 P3 属性显示重复修复

- 用户授权修复上述P3；新增UI层`WizardAttributeDisplay::GetLabel`，集中维护属性中文名称，背包装备详情与商店商品详情共同调用。沿用原背包的八种名称及未知属性回退引擎名称的约定；规格允许配置的五种装备属性显示保持一致。未改资产、交易规则或输入路径。
- Standards：重复名称映射已移除；纯展示函数无UObject持有、委托或反射接口，命名与生命周期检查通过，P3关闭。此类低风险重构按编码规范第6.1节不新增长期测试。
- Spec：R23～R25及装备属性显示范围保持；工程回归通过不替代既有待用户运行验收，T07继续in-progress。
- 用户保存并关闭编辑器后，`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor`退出0；实际增量编译共享函数、背包及商店三个cpp并链接，非Live Coding、非干净全量重编译；证据`Saved/T01/20261101-211359-BuildEditor-743efd/`。
- `pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Test -TestFilter Wizard.Trading`退出0，5 succeeded/0 failed/0 notRun，证据`Saved/T01/20261101-211443-Test-ee99bc/AutomationReport/index.json`；仅证明交易专项回归，不声称测试覆盖显示文案或实际页面。静态核对两处调用共用名称映射，`git diff --check`通过（仅LF/CRLF提示）。
- 未执行PIE、Cook或打包；未提交/推送，保留其余已有改动。

## 2026-11-01 用户完成确认与版本交付

- 用户明确“T07完成，提交并推送”，据此接受商店功能交付，勾选验收并转done；用户掌握程度仍未评估。未新增或更改规格要求。
- Standards：交易原子性、失败边界、会话及生命周期已审查，P3已关闭；沿用本票据已有构建、自动化及编辑态资产证据，不为文档收尾重复构建或运行效果检查。
- Spec：本任务功能由工程证据和用户完成确认共同验收；存档落盘/重启往返归T08，独立包商店资源加载和完整循环归T09/T10，未声称本轮执行PIE、Cook或打包。
- 版本范围：T07源码、五项长期交易测试、商人/商店资产、十五份物品交易配置、开发地图及相关规格/说明纳入同次提交。地图保留现有手工布局并包含商人入口；不重跑布景。T01票据、本机`Config/DefaultDefaultEngine.ini`和卡顿调查文档保留在工作区，不随T07提交；本地学习资料保持忽略。
