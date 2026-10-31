# T04 物品模型与背包

- Status: done
- Blocked by: [T01](./01-environment-and-quality.md)、[T02](./02-basic-combat.md)
- Spec: R03、R04、R09、R10、R11、R13、R14，正文见 [规格](../spec.md)
- Updated: 2026-10-31

## 目标与范围

建立物品定义/运行实例、20 格背包和可验证的事务接口，完成堆叠、拆堆、使用、背包内拖拽与数量操作界面。丢弃生成地面对象由 T05 接入，装备跨槽拖拽由 T06 完成。

## 输入、输出与不变量

- 输入：物品定义/实例、来源与目标格、数量、使用请求。
- 输出：可区分的成功/失败结果、一次提交后的物品状态及 UI 事件。
- 数量守恒，每件运行实例只有一个归属；装备不堆叠，上限由定义配置。保存所需字段可序列化。

## 失败与边界

检查负数/零/越界数量、缺失定义/实例、非法位置、背包满、合并溢出、无效或取消拖拽、关闭界面时拖拽。对应生命/法力已满时使用药水失败且不扣数量。

## 验收条件

- [x] 定义、实例身份和位置分离，公开事务接口能被独立验证。
- [x] 20 格、配置堆叠上限、背包内换位/交换/合并成立，溢出留在来源。
- [x] 拆分采用独立数量弹窗；丢弃入口采用明确按钮，接口支持 T05 安全提交；交易界面由 T07 实现。
- [x] 药水使用与 T02 的真实属性/效果接通，满值失败不消耗。
- [x] 暂停中的背包交互、拖拽取消与事件解绑正确。
- [x] 事务成功/失败、数量守恒与 UI 交互均有验证；持久字段边界交给 T08 使用。

## 验证记录

2026-10-31 用户确认当前编译验证已通过、背包功能正常，接受本票据功能交付；T01/T02前置已完成。历史工程证据与各轮适用边界见下文，最终状态以末尾完成记录为准。
Standards：源码、事务/MVC边界及本轮清理符合已确认项目规范；清理后编译通过由用户确认，Agent未重复执行。Spec：T04范围由既有业务验证与用户运行验收共同确认。资源加载检查按用户本轮要求暂缓，不作为T04完成阻塞，不宣称Cook或独立包验证通过。

## 未决实现与学习

定义资产格式、实例 ID 表示与保存快照接口在此落定，参考技术提案。讲解配置/实例/位置、事务和守恒。

## 2026-10-31 开工

- T01/T02已done；本次执行T04，不提前实现T05地面生成、T06装备、T07交易或T08落盘。
- 用户确认Tab开关背包，Esc关闭最上层；键位通过Enhanced Input资产配置。
- PlayerState持有背包组件；定义、FGuid实例与20格位置分离，revision与实例身份校验旧请求；事务成功一次提交，失败不修改来源。
- CommonUI复用Menu层与集中暂停；布局/资产通过UE-MCP配置，C++负责业务、事件和生命周期。
- 当前Standards/Spec尚未完成验证，不因开工标done。

- 用户补充背包沿用MVC：Model为InventoryComponent；独立UObject InventoryMenuController管理会话、选择令牌、数量提交、事务转发和订阅；Widget仅负责显示、CommonUI生命周期与指针事件转发。控制器独立于Slate可测。

### 本轮实现与工程验证

- `Inventory/WizardItemDefinition` 定义稳定ItemId、类型、堆叠上限、图标软引用和药水资源/恢复量；装备配置上限必须为1。`WizardInventoryConfig`持有定义硬引用与可配置初始库存。`WizardInventoryComponent`常驻PlayerState，InitializeAvatar重复调用不重发；快照保存定义ID、FGuid身份与数量，位置是20格数组索引。
- 请求携带格位、实例ID与revision；先校验再一次提交。拖拽增加独立菜单SessionId，数量输入保留选择时请求；关闭/重开或外部变更后旧请求无效。恢复拒未知定义、重复GUID、非法数量；无变化不广播。
- 药水复用真实GAS Instant GE（Heal/Mana），Spec按定义注入恢复量。只开放已知资源类型，避免任意GE副作用破坏库存事务；属性事件执行期间拒背包重入，满值/死亡/失败不扣量。T05通过PrepareDrop/CommitDrop可靠交接；当前按钮明确反馈地面未接入且保留物品，不生成地面物品。
- MVC：Model=InventoryComponent；Controller=WizardInventoryMenuController，独立UObject管理会话、选择、业务转发及结果映射；View=Menu/Tile，原位刷新避免按下选择销毁拖拽源。数量用EditableTextBox严格解析十进制int32，避免SpinBox浮点舍入。CommonUI注册同一InventoryAction关闭，世界Enhanced Input负责打开。
- 首轮Build失败：新Widget的Slot局部名遮蔽UWidget::Slot，且UHT期间header修改导致生成行宏不一致；已统一Index命名，补齐DataValidation后冻结源码重新构建。失败证据：`Saved/T01/20261031-005647-BuildEditor-8c26e6`，退出6，不计通过。
- UHT/UBT：`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor`，`Saved/T01/20261031-005834-BuildEditor-9f8886/result.json`；实际13项编译/链接动作，退出0，非Live Coding，不是干净全量重编译。
- 全量自动化：`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Test`，`Saved/T01/20261031-005854-Test-ec821e/result.json`与AutomationReport/index.json；19成功、0失败、0未运行，退出0。新增`Wizard.Inventory.TransactionsAndRestore`、`ConfigurationAndAtomicInitialization`、`PotionsAndReentry`和`Wizard.Inventory.MenuSession`。覆盖守恒/溢出/部分容量/MAX_int32、非法配置/整批初始化、快照恢复、真实ASC/GE/属性事件重入、旧会话/延迟请求/关闭解绑。
- UE-MCP创建并保存：IA_Inventory、IMC_Combat的Tab映射、WBP_InventoryMenu/Tile、DA_HealthPotion/ManaPotion/ArcaneOre/ApprenticeStaff和DA_InventoryConfig。初始数量15/6/27/2，占7格；法杖两件不堆叠。仅新PlayerState初始化一次，可配置并为未来读档全量替换保留边界。
- MCP `editor.validate_assets(directory=/Game/Wizard/Data/Items)`：5 checked/5 valid/0 invalid/0 warning；Menu build_tree编译upToDate，check_structure pass/0 failure。图标暂复用现有Free_RPG_Icons Holy/Water/Earth/Fire作为验证占位，未导入Lorc素材；不将其当最终物品美术验收。
- 编辑器重启加载落盘资产后进入现有L_WizardClearing PIE；只读GetSnapshot读回20格及正确样例定义，库存revision已随用户操作增长。MCP窗口截图`Saved/T04/inventory-pie.png`含UI。用户选择自己试玩，自动化没有发送CUA输入；物理Tab/Esc/焦点/拖拽/数量输入与暂停隔离矩阵待用户反馈。没有重置覆盖用户正在玩的PIE状态。
- Cook/Windows包与成品资源加载本轮尚未执行；按减少打包约定集中阶段验证，当前用户试玩期间不关闭编辑器。

### 当前双轴结论

Standards：本机UE5.4 UHT/UBT及19项自动化通过，DataValidation与Widget结构通过；物品事务、GAS唯一属性、CommonUI生命周期/MVC分工、弱引用与解绑、资产MCP操作符合本次项目约定。物理输入与成品资源可达性仍待实际验收。

Spec：R09/R10/R11/R13/R14的公开业务规则与失败路径已有自动化证据，真实药水效果通过；R03/R04及背包实际交互等待用户试玩反馈，Cook/成品资源未验证。T04保持in-progress，未提前勾选依赖实际UI和完整资源验证的条目。T05地面丢弃为后续任务，安全接口失败不扣已有验证。

学习材料：`.local-learning/lessons/2026-10-31-inventory.md`、`.local-learning/assessments/2026-10-31-inventory.md`，学习计划已追加；掌握度未测试。未提交/推送，保留用户未跟踪Config/DefaultDefaultEngine.ini。

## 2026-10-31 用户试玩后的UI迭代（实现中）

- 用户要求正方形图标格、无名称、数量右下叠加；可操作悬浮详情内使用/丢弃/拆堆；移除固定底部详情和按钮。数量弹窗复用现有Modal层，不新增层；Esc先关Modal，取消或过期请求不提交。
- 复用评估：技能只读引擎tooltip与背包可交互popover拥有不同焦点和生命周期，不合并业务。两个真实调用者共用图标拖拽预览Module，Interface为Configure/SetVisual；业务request和MVC控制器保持独立。旧类CoreRedirect保证旧预览WBP可在迁父类之前冷启动加载，迁移仅通过MCP，保留技能布局与行为。
- 数量操作打开时冻结实例ID、revision、SessionId及Split/Discard操作，确认不读取后来Selected决定来源；新增FrozenQuantityRequest自动化源码。数量输入仍严格十进制int32，无浮点舍入。
- 悬浮窗由Menu管理鼠标源格和窗体范围，短宽限允许跨间隙进入操作；开始拖拽/打开Modal/关闭页面清理，键盘Enter/Space可展开并焦点进入按钮。
- 本段新增源码尚未编译/运行验证，不能沿用上一版本19项通过结果当本次通过。

### 第二轮工程与PIE结果

- 冷UHT/UBT首轮`Saved/T01/20261031-011828-BuildEditor-141fba`因局部Cursor遮蔽UWidget::Cursor失败（退出6）；统一改MousePosition。最终`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor`通过，证据`Saved/T01/20261031-011918-BuildEditor-1c0445/result.json`，退出0，实际重新编译链接。新增反射类已由前轮UHT生成，未使用Live Coding。
- 全量自动化`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Test`：`Saved/T01/20261031-012052-Test-9efb2f/result.json`，20成功/0失败/0未运行，退出0；包含新增`Wizard.Inventory.FrozenQuantityRequest`，验证打开数量操作后改选仍按原实例、外部更新/跨会话/重放拒绝、取消及失败不改状态。
- UE-MCP保存WBP_InventoryPopover、WBP_InventoryQuantityModal，重建InventoryTile/Menu布局并设置QuantityModalClass；WBP_SkillDragPreview通过CoreRedirect在冷启动可加载，MCP reparent通用WizardIconDragPreview，保留原外观。5份受影响Widget DataValidation全部valid，0warning。
- 首张PIE截图发现数量仍左上：MCP slot枚举便捷入口接受Right/Bottom而非HAlign_Right/VAlign_Bottom；已写入并读回真实HAlign_Right/VAlign_Bottom，新PIE截图`Saved/T04/inventory-square-final.png`确认正方形格、无名称、右下数量与移除固定详情/操作区域。
- MCP公开UIRoot路径实际验证：OpenInventory=True且GameplayStatics.IsGamePaused=True；PushModal(WBP_InventoryQuantityModal)返回真实实例；CloseTop关Modal后HasOpenScreens=True、IsGamePaused=True；再次CloseTop关Menu后IsGamePaused=False。`Saved/T04/inventory-modal-stack.png`仅证明真实模态堆栈和布局；此调用未配置物品令牌，不代替实际从popover拆堆入口、编辑数量及确认验收。
- 结束上述生命周期检查后库存仍Revision=1、初始生命药水15/法力药水6/矿石27/法杖2，世界解除暂停、菜单关闭，PIE留给用户手动试玩。没有发送CUA键盘/鼠标，不抢用户输入。
- 当前Standards：第二轮真实编译、20项自动化与5Widget配置验证通过，真实Modal嵌套暂停成立。Spec：正方形/右下数量视觉及Menu/Modal生命周期通过；hover跨窗稳定性、popover按钮、实际数量输入/确认/取消、Esc/Tab/焦点/拖拽取消仍待用户手动反馈；Windows Cook/包未执行，整票维持in-progress。使用药水成功时现有同步刷新会收起详情，下一次悬停可重开；不将此当连续点击保持窗体已验。

## 2026-10-31 数量双控件与草图布局调整

- 用户最终确认拆分范围1～当前数量−1（覆盖先前0下界），丢弃1～当前数量；输入+Slider双向同步整数，文本精确值不被浮点回写；失焦/确认恢复空或非法暂态。标题统一背包/拆分/丢弃，不附范围或说明；已删除背包Controls控件及占位。
- 双控件实现已实际UHT/UBT通过：`Saved/T01/20261031-164640-BuildEditor-c8e559/result.json`，退出0；实际10项编译链接。用户随后反馈“拆分功能正确”，本轮继续仅调整布局。
- 相关自动化`Saved/T01/20261031-165015-Test-ed860f`中的新增QuantityWidgetControls失败：headless夹具未加载CommonInput数据，且把程序SetText误当用户OnTextChanged。该夹具失败不作为功能缺陷；按用户新约束撤除本轮自有未跟踪WizardInventoryQuantityTests.cpp与未编译的WizardInventoryQuantityPreview.cpp，不再增加运行验证命令、不继续重编或复跑。保留既有20项已通过业务测试及历史证据；不宣称新增测试通过。
- 用户明确运行时效果验证全部交由用户。收到后停止所有自主PIE、运行截图和控件效果验证。本次草图布局只通过UE-MCP编辑默认资产，不关闭/重置当前试玩。
- WBP_InventoryQuantityModal默认布局：面板580×260，内边距24/22；左上标题；中行Height56，输入/Slider同HorizontalBox，Fill比例0.28/0.72，间距20；Slider BarThickness10，thumb20×36；底部确认/取消各100×42居中横排，间隔24。只修改布局/style，不改已确认的数量同步和业务。
- 编辑态资产保存成功，属性读回输入Fill0.28、右间距20、Slider Fill0.72；MCP批量结构值入口未生效时改用单项UE export text写入。没有运行截图，不声称运行效果通过；下次新建弹窗实例采用新布局。
- 当前Standards按编码规范4.1检查：标题、玩家用语、删除常驻说明、双控件生命周期与整数守恒按本轮确认实现；编辑态布局保存与git diff --check通过。Spec：数量功能用户已反馈正确；新横向草图布局运行效果等待用户验收。T04仍in-progress，未打包/提交/推送。

## 2026-10-31 用户背包验收与临时装配入口清理

- 用户反馈“背包功能可以了”，记录为本轮背包交互与布局可用的用户运行验收；不据此提升尚未执行的Cook、独立包或后续经济/存档集成状态。
- 按用户要求清除已完成用途的一次性反射装配类：WizardEnemyAssetTools、WizardCombatAssetTools、WizardChallengePresentationTools、WizardChallengeAssetTools、WizardBarrierAssetTools，共10个h/cpp；同时删除直接调用旧入口的一次性脚本Tools/Combat/setup_combat_assets.py。历史票据中的调用记录只代表当时来源，入口现已移除，不再可执行。未将源码搬至新类或Editor模块，未修改已保存游戏资源。
- Wizard.Build.cs删除仅供这些装配实现使用的UnrealEd、UMGEditor、Kismet、KismetCompiler、AssetRegistry、AnimGraph、BlueprintGraph、AnimGraphRuntime八个编辑器依赖；保留环境配置测试使用的EngineSettings。
- 源码/配置/脚本静态检索未发现其他游戏代码调用这5类；轻量扫描Content/Wizard下77份uasset/umap的类名与函数名UTF8/UTF16字节，未命中。此为有限静态证据，不等同完整加载或资产引用验收。
- 未发现专门调用这些装配入口的配套回归测试；本次不新增清理检查器或回归测试。保留真正玩法与事务测试，以及具有现有构建脚本/文档调用链的4份Smoke检查，不因名称或方法数误删。
- Standards：一次性源码/反射入口及独有依赖已清理，符合编码规范2.1；`git diff --check`通过（退出0），Source/Config/Tools中已无这5个工具类的调用。Spec：保留已生成资产和用户改动，不改变玩法范围；背包反馈与成品/集成验收分开记录。
- 本次未运行UHT/UBT、自动化、PIE、Cook或Package。当前用户编辑器仍打开（调查时PID234936），没有关闭或操控用户会话；必要编译留待编辑器正常关闭后执行。未提交/推送，保留Config/DefaultDefaultEngine.ini等既有改动。T04保持in-progress，不能用此次清理代替未完成的成品资源验证。

## 2026-10-31 按用户要求删除低价值测试片段

- 删除菜单测试中仅执行局部令牌赋空的假取消检查，以及断言T05尚未接入、丢弃必定不可用的阶段性检查；保留真实会话/旧请求拒绝及Model接收失败不扣物品的事务检查。不新增替代测试，完整清理范围见[T01本轮记录](01-environment-and-quality.md)。
- 纠正此前工程证据的覆盖描述：`FrozenQuantityRequest`中局部变量赋空不能证明真实弹窗取消正确，原20项通过的历史记录不作为该行为的自动化证据；背包运行效果采用用户已反馈可用的验收。本次不改变玩法或任务状态，未执行构建或测试。

## 2026-10-31 完成确认与版本交付

- 用户明确反馈“编译验证已经过了”“我确认过功能正常”，本记录采用用户提供的当前版本编译和运行验收结果；未提供新的构建日志路径或退出码，Agent不补造，也不重复构建或试玩。
- 用户明确资源加载先不用检查，本轮暂缓该项，不将其记录为通过。T04范围内验收完成，状态转done，解除T05/T06对T04的前置等待；地面丢弃和自动拾取仍归T05，装备归T06，落盘存档归T08。
- 本轮同时包含已确认的UI文案/布局、临时装配入口清理和六类低价值测试删除；保留必要业务测试，未新增替代测试。自动化未在最后清理后重跑，历史运行结果仅用于对应版本和规则的证据。
- 用户授权提交并推送当前功能及相关规范清理。提交范围为本次源码、落盘资产、必要配置、工程工具与文档；用户原有未跟踪`Config/DefaultDefaultEngine.ini`保留本地，不纳入本次提交。
