# T03 六技能与搭配

- Status: in-progress
- Blocked by: [T02](./02-basic-combat.md)
- Spec: R03、R04、R05、R06、R07、R08，正文见 [规格](../spec.md)
- Updated: 2026-10-29

## 目标与范围

按规格验证六种候选主动机制，完成四槽技能搭配与营地调整界面。资源选择通过实际预览落定，行为变化按规格变更流程处理。

## 输入、输出与不变量

- 输入：1234 对应槽位、当前搭配、光标目标和技能配置。
- 输出：经 GAS 执行的能力、消耗/冷却和反馈。
- 六技能开局解锁、最多四槽、同技能不重复，仅在营地调整。输入解析槽位再请求 Ability，与具体技能类解耦。

## 失败与边界

覆盖资源不足、冷却中、无效目标、无效闪现落点、施法取消/死亡、连续输入、营地外改搭配与重复装备。关闭界面、角色重建后无残留输入或任务。

## 验收条件

- [ ] 六机制及资源映射经实际预览确认；与规格不同的机制先完成变更确认。
- [ ] 四槽可配置且通过 1234 真实施放，调整搭配不修改按键处理代码。
- [x] 营地/重复限制、消耗/冷却、激活失败、正常结束和取消都可验证。
- [x] 属性上下限、护盾吸收/到期、减速清理、治疗和闪现落点规则有对应验证。
- [ ] 技能界面暂停、返回、焦点和输入隔离正确；显示实际 GAS 状态。
- [ ] 相关构建/自动化、实际输入、资源表现和独立包内资源加载有证据。

## 验证记录

未执行本票据功能验收。素材结构检查不代表能力已实现。
Standards：未执行。Spec：未执行。

### 2026-10-29 首轮实现与验证

- 前置T02当前版本真实输入由用户确认“全部正常”，现版独立包CombatSmoke通过，T02已done。
- C++：技能定义/目录、PlayerState搭配组件、五种可复用行为覆盖六技能；GE管理消耗/独立冷却/伤害/减速/护盾/治疗，Montage+原生Notify+AbilityTask管理出手与取消。搭配提供快照/恢复/有效变更事件，存档磁盘写入不属于本票。
- UI：Menu层加入Alchemy技能搭配页；候选拖拽替换、重复拒绝、槽位交换立即提交，HUD独立模块显示实际GE冷却与法力不足。现有左下生命/法力保留，新增技能栏置于右下以避让小窗口。
- 资产全部经UE-MCP：火球Projectile1_Base/Hit1项目副本；冰箭Projectile2_Base/Hit2项目副本，参考Area1后设置冰蓝RGB(.12,.65,1)归一比例，保留各参数原HDR峰值/Alpha及所有非颜色数据；Niagara编译分别12/14脚本全UpToDate，0错误。雷击使用Cascade P_ky_lightning3，闪现/护盾/治疗引用Free_Spells。没有引用ArrowTrail，普攻资产未改。
- 现有Camp_Alchemy读回位置(-1592.924,-631.400,0.011)，新增无视觉AlchemyStation引用该对象，并配置营地盒体；没有移动或替换原布景。技能菜单、卡片、HUD、1234/E映射、6份定义与目录、两份GC蓝图均经MCP创建/保存。
- UHT/UBT通过：`Saved/T01/20261029-141206-BuildEditor-e652f7`首轮、`141410-BuildEditor-48c0cc`UI集成、`142119-BuildEditor-e549ef`测试修正及`143048-BuildEditor-d2f8cc`取消检查扩展。全部为实际编译/链接，非LiveCoding或干净重编译。期间Slot局部变量遮蔽与TObjectPtr测试参数编译问题已修复，不抹除失败日志。
- 自动化：`Saved/T01/20261029-142129-Test-4ddcd1/AutomationReport/index.json`，6 succeeded/0failed/0notRun。新增DefinitionsAndLoadout覆盖数据有效性、授予幂等、非法快照原子性、营地限制、换槽/换Avatar保留冷却及满血/耗蓝失败；EffectsAndExpiry覆盖实际ASC伤害、护盾吸收/溢出/刷新/到期/死亡清理、治疗夹取、减速不叠乘与实际移动组件恢复。
- 首轮测试失败：旧普攻用例将能力总数固定0，六技能加入后改为断言“不授予无效普攻”；到期夹具最初在同帧Tick，TimerManager未推进，已按真实引擎帧边界推进后保留原断言重验通过。
- PIE业务Smoke：`Saved/Logs/Wizard.log`中2026-10-29 14:23:36～14:23:45记录六项WIZARD_SKILL_CHECK Passed=1及WIZARD_SKILLS_SMOKE_PASS。实际Notify驱动投射物/范围/位移/护盾/治疗，扣蓝分别12/14/22/18/20/20；Alchemy范围/交换/重复与页面暂停恢复通过。只临时排除后台CPU/Slate节流，停止PIE后恢复原值。
- 实际输入/拖拽：CUA真实E打开页面；治疗候选拖至第四槽成功且HUD同步；火球候选重复拖入被拒绝并显示原因；第一/第二槽实际拖拽交换成功。页面保持暂停，布局与中文文本可见。PIE的Escape属于编辑器停止快捷键，独立包Esc与1234矩阵继续单独验收。
- 首轮Windows包：`Saved/T01/20261029-142740-Package-21e068/result.json`退出0，实际Game编译、Cook 0errors/0warnings、Stage/Package/Archive成功。`Smoke -SkillSmoke`证据`Saved/T01/20261029-142930-Smoke-ff4c4d`退出0、bridgeLoaded=false，真实六技能与菜单通过；`Smoke -CombatSmoke`证据`143012-Smoke-d91327`退出0，原普攻完整回归通过。
- 进一步验证：补充显式取消/UI/死亡/换Avatar四条出手前取消路径及不退款断言，正在生成包含这些检查的最终包。最终自动化/包/实际输入结果以随后记录为准，当前未将整票标done。

阶段Standards：现有构建、6项自动化、MCP资产流程及首轮独立包通过；最终变更复验仍在执行。阶段Spec：六机制与实际E/拖拽通过；最终独立包1234、Esc/焦点、动态音画确认及取消扩展证据待补齐，不以业务Smoke替代物理输入。

### 最终复验进度（2026-10-29）

- 最终统一Test：`Saved/T01/20261029-143208-Test-a99530/AutomationReport/index.json`，6 succeeded/0failed/0notRun，退出0。
- 最终Package：`Saved/T01/20261029-143229-Package-1b0cdb/result.json`，退出0，实际Game目标编译、Cook 0errors/0warnings、Stage/Package/Archive成功。成品`Saved/Packages/20261029-143229-Package-1b0cdb/Windows/`。与上一包的玩家行为/资源一致，新增四条取消Smoke并移除未使用的请求目标缓存。
- 扩展Smoke首轮：`Saved/T01/20261029-143414-Smoke-e250e5/packaged-skills-smoke.log`，脚本退出1（PASS标记缺失；游戏进程退出0不当成功）。六项技能检查及显式取消Path0通过，后因60秒墙钟限制失败。当时同时运行用户试玩实例；启动PSO/资源等待占据大量墙钟，Fireball首项直到14:35:26才完成，14:35:43触发超时。尚未覆盖本轮UI/死亡/Avatar取消，不能写成通过。将保持原断言/超时，在单进程条件复验。
- 已为用户打开上一份相同玩家行为/资源的T03包，待1234、E、Esc、拖拽及音画反馈；避免与用户抢输入，后续未再使用CUA控制该窗口。
- NullRHI补充验证：`Smoke -SkillSmoke -SmokeNullRHI -PackageRoot Saved/Packages/20261029-143229-Package-1b0cdb`，证据`Saved/T01/20261029-143839-Smoke-6aac16/result.json`，退出0，renderingEnabled=false、bridgeLoaded=false。六技能与四条取消路径全部通过：Path0显式、Path1 UI、Path2死亡、Path3 Avatar，均SpawnDelta=0/NoRefund=1/Casting=0。此项不证明粒子像素、音效听感或渲染性能。
- 新增`Wizard.Skills.TargetsAndCampTransactions`使用真实碰撞世界，验证开放闪现、距离截取、墙体/无地面/零方向拒绝、雷击遮挡/超距，以及营地内替换/交换/重复拒绝和离开交互范围失败原子性。交互夹具改用有实际几何中心的Alchemy代理，并在组件注册后推进帧再检查，避免根为空或新刚体未同步的夹具错误。`144010-Test-82d6ee`定向通过，最终`Saved/T01/20261029-144120-Test-d41c77/AutomationReport/index.json`7 succeeded/0failed/0notRun，退出0；前置实际编译`144113-BuildEditor-dc7849`退出0。新增内容仅WITH_EDITOR测试，不改变成品运行时逻辑。
- 工程说明`docs/SKILLS.md`与构建入口`-SkillSmoke`已补齐；学习材料在`.local-learning/lessons/2026-10-29-skills-and-loadout.md`，用户掌握度未测试。README保持空白；未提交/推送，保留既有Config/DefaultDefaultEngine.ini。

### 本轮交付状态（2026-10-29）

- 已实现六技能、四槽、Alchemy/E、拖拽立即替换/交换、重复拒绝、真实冷却/法力反馈、暂停/输入取消和快照接缝；具体源码与资源入口见`docs/SKILLS.md`。
- Standards：当前C++ UHT/UBT、7项自动化、MCP资产装配、Cook/Package与diff-check通过；新资产匹配Git LFS规则，原README仍0字节、未改用户的未知配置。动画蓝图继续Blueprint，聚合HUD无业务图表，业务在C++，没有使用ArrowTrail。
- Spec：公开规则、六技能实际Montage/碰撞/GE与四条取消、实际E及拖拽检查已通过。独立包物理1234/长按单发、Esc/焦点、全部技能音画听感仍待用户反馈；因此T03保持in-progress，未提前勾选依赖完整人工验收的条目。
- 14:42的再次有渲染复验`Saved/T01/20261029-144201-Smoke-af5505`仍与用户试玩进程并行；进程摘要格式曾让执行者误判已关闭，随后核对完整PID确认试玩实例仍在。该轮出现启动RHI/资源等待，没有新增PASS，不算通过；只结束本次持有且路径验证匹配最终包的测试PID230748，保留用户试玩PID226772。没有修改渲染设置、放宽60秒断言或终止用户游戏。
- 可复跑当前最终包：`Saved/Packages/20261029-143229-Package-1b0cdb/Windows/Wizard/Binaries/Win64/Wizard.exe`。`Smoke -SkillSmoke -SmokeNullRHI`是完整业务/取消通过证据；普通有渲染六技能通过证据为首轮T03包`142930-Smoke-ff4c4d`，不混淆两者覆盖范围。最后新增的是WITH_EDITOR碰撞测试，运行时功能与最终包一致。

## 未决实现与学习

### 滚动技能网格、空槽与卸下（2026-10-29）

- 用户本轮授权规则细化已同步规格R05及技能段落：四槽允许部分/全部为空，装备槽拖回上方列表即卸下；已装备候选压暗且不可拖拽；预览只显示图标，恢复“炼金台”标题。
- 业务：增加UnequipSkill，先验证营地/距离/存活/有效目标和槽位，再通过RestoreSnapshot原子清空。快照保持四元素，以NAME_None表示空槽，重复空值合法，重复非空/未知ID仍拒绝；初始目录配置也允许空位。卸下不移除能力或GE，冷却保留；全空搭配不会因重复初始化被默认搭配覆盖。空槽ActivateSlot提前返回InvalidSlot，不扣资源或打断普攻。
- UI：候选使用CanDragSkill在按下、开始拖拽和提交时校验，已装备候选保留悬浮提示但不能发起拖拽；槽位拖拽核对来源页/槽/ID。菜单NativeOnDrop仅在LibraryScroll几何范围内接收装备槽卸下，包含候选卡片及空白，其他区域/取消不修改。空槽隐藏图标与技能名，HUD/菜单保留数字，不误显示法力不足。
- 所有资产经UE-MCP：Menu上方LibraryFrame(720×335)/LibraryScroll/UniformGridPanel，按index/6和index%6排六列，滚动条显示；标题“炼金台”30号，下方槽位锚点.5/.86。新增WBP_SkillDragPreview(78×78 SizeBox+Image)供DragPreviewClass，只有图标。最终Menu/Tile/Preview编译保存3成功/0失败。替换旧Candidates时中间编译曾缺少LibraryScroll，补齐后最终绑定/编译正常；未改地图、技能数据或输入映射。
- UHT/UBT `Saved/T01/20261029-170404-BuildEditor-87e5e7`退出0；补充测试断言后实际编译`170503-BuildEditor-1ddfb2`退出0。统一Test `170953-Test-33a6f6/AutomationReport/index.json`退出0，8 succeeded/0failed/0notRun。新增断言覆盖全空快照/重复初始化不回填、空槽请求/法力不变、卸下/重复卸下幂等事件、非法槽、技能交换到空槽、候选装备到空槽/重复拒绝、营地外卸下失败原子性。
- PIE最终截图`Saved/T03/SkillMenu/grid-final.png`确认标题、六列网格滚动区域与下方槽位，LibraryScroll实际708×323且无父边界越界。通过公开接口UnequipSkill(0)=Success，`empty-slot.png`确认上下列表/HUD立即刷新：火球恢复亮色，空槽保留1且无错误法力文字；EquipSkill(Fireball,0)=Success恢复。该轮未注入物理拖拽，不把接口检查或资产树检查当作拖拽/preview实操已验收；六技能目前恰好一行，更多行采用索引规则，未为展示添加假技能。
- Standards：按编码规范第2/4/6节，状态由业务接口原子修改、失败不广播、布局/预览来自BP、拖拽来源重验与取消保留通过代码检查及相关回归。Spec：本轮规则、布局/空槽显示通过对应自动化与PIE，灰色禁拖/图标预览/滚动区卸下的物理拖拽组合仍待试玩确认。停止PIE后恢复临时后台CPU/Slate节流，无脏包；不Cook/打包，旧包不含本次改动。T03仍in-progress，未提交/推送。

### 技能搭配页面精简与悬浮详情（2026-10-29）

- 按用户文字及截图批注：移除MenuTitle/Instructions与固定Details；技能列表不再显示“已装备/拖入槽位”，已装备图标复用HUD不可用状态RGB(.35,.35,.35)压暗，未装备保持原色；已装备技能列表移至内容区中下方居中（锚点.5/.73，自动尺寸、居中对齐），保留技能名与1～4，去除“拖动交换”。上部列表居中，错误反馈仍可显示。
- C++：SetStatus对空文字Collapsed；Refresh按真实Snapshot更新压暗，装备槽保留按键数字；共用原EquipSkill/SwapSlots、重复拒绝及刷新事件。新增UWizardSkillTooltip仅从定义读取名称/描述/法力/冷却；菜单卡片使用引擎SetToolTip，在悬浮时显示，HUD与拖拽影像不创建。移除点击显示固定详情的旧接口。
- 资产修改全部通过UE-MCP：MenuLayout Canvas配置上下列表锚点；WBP_SkillTooltip为深色Border/内边距/TextBlock字号18与360换行宽度，WBP_SkillTile配置TooltipClass。Tooltip/Tile/Menu三项编译保存成功、0失败，未修改地图或输入映射。
- 实际UHT/UBT `Saved/T01/20261029-164908-BuildEditor-e96958`退出0；统一Test `165312-Test-f31758/AutomationReport/index.json`退出0，8 succeeded/0failed/0notRun。未增加仅镜像布局的测试。
- PIE `Saved/T03/SkillMenu/layout.png`确认上部四个已装备图标压暗、其余原色，下部四槽在页面中下方居中，仅显示名称和数字；无旧标题/说明/固定详情。`hover.png`/`hover-delay.png`确认引擎悬浮框显示火球名称/说明/法力12/冷却2.0秒，描述正确换行，底板包裹文字。期间槽位发生交换后HUD与下列表同步显示；不把该观察当作本轮自动物理拖拽验收。一次接口检查因使用错误参数名SkillId返回UnknownSkill，没有修改搭配；真实参数名为Id，未用临时校验改动覆盖当前试玩状态。
- Standards：UI显示业务与蓝图布局分工、配置引用、悬浮提示生命周期及diff检查通过；Spec（R03/R04/R06及本次用户布局要求）：页面显示/悬浮详情通过PIE，搭配限制与生命周期通过既有8项回归。本轮未重新验证完整物理E/Esc和拖拽矩阵。已停止的PIE无脏包，恢复临时后台CPU/Slate节流；按用户要求不Cook/打包，旧包不含本轮修改。T03仍in-progress，未提交/推送。

### 交互提示背景框与减少打包（2026-10-29）

- 用户反馈频繁打包导致UE严重卡顿，要求减少打包；已同步docs/agents/validation.md，小范围UI修改优先Widget编译保存/PIE，Cook与Windows包集中到阶段交付或明确资源加载风险时验证。本轮不诊断或声称解决卡顿根因，不执行Package/Cook。
- 经UE-MCP保留既有PromptText，在外层增加PromptFrame Border：深色半透明底(R=.025,G=.035,B=.05,A=.85)，左右12/上下6内边距，自动尺寸、底部居中。C++位置与显隐改为控制整个框体，文字保留原样式，避免只隐藏文字后遗留空背景；框与文字均HitTestInvisible。
- 实际UHT/UBT `Saved/T01/20261029-163918-BuildEditor-d075c4`退出0；相关既有Wizard.UI回归 `163950-Test-31da44/AutomationReport/index.json`退出0（未新增镜像布局的测试）。MCP编译保存Prompt/CombatHUD：2成功/0失败。
- PIE截图`Saved/T03/InteractionPrompt/background-frame-final.png`确认背景框包裹文字、位于模型上方。运行树框175.64×47.42、文字151.64×35.42，无布局越界或诊断；菜单打开时框Hidden，CloseTop后恢复HitTestInvisible；移出范围后框Hidden，无残留底板。停止PIE后恢复临时CPU/Slate节流，dirtyPackages为空，编辑器保留打开。
- Standards：业务控制显隐/定位，蓝图配置样式，MCP修改资源、编译/相关回归及diff检查通过。Spec：本次背景框要求与R03/R04/R06相关提示显隐经PIE确认；未重新测物理输入/拖拽，未打新包，旧成品不含本次修改。T03仍in-progress，未提交/推送。

### 交互提示移至交互物上方（2026-10-29）

- 按用户要求将提示从技能栏移出，增加独立`UWizardInteractionPrompt`/`WBP_InteractionPrompt`，仍属于HUD，不接管焦点和点击。Alchemy和未来Shop共享当前交互目标与提示事件，不增加业务分支或交互轮询。
- 目标仅提供可见Mesh的世界包围盒，排除交互查询球；界面投影八角取屏幕顶端居中、底部对齐并保留间距，适配俯视角、镜头跟随和DPI。可配高度偏移默认30cm。无模型回退对象原点；投影失败或锚点在屏外隐藏。每帧仅更新当前目标的视觉位置，不搜索候选或查询遮挡。
- 所有Widget资产经UE-MCP创建/编辑/编译保存：CombatHUD内提示控件四边拉伸零偏移，原SkillBar.InteractionHint移除；PromptText自动尺寸、居中底对齐、白字20/描边2、HitTestInvisible，默认Hidden。地图和用户布局未改。
- 首轮编译发现局部Slot/Bounds遮蔽UE成员（C4458），重命名修复；`161355-BuildEditor-09278b`通过。首轮视觉顶部中心仍压在炼金台上沿，改为整个包围盒投影后，最终实际UHT/UBT`Saved/T01/20261029-161659-BuildEditor-cc9b93`退出0。`Test`报告`161730-Test-ee9eb2/AutomationReport/index.json`为8 succeeded/0failed/0notRun，退出0。
- PIE最终预览`Saved/T03/InteractionPrompt/above-alchemy-final.png`：提示位于模型上方；移动到(-1412.9,-731.4,105)后镜头跟随，`moved-camera.png`确认提示仍跟随炼金台。运行树读回Text为“E　技能搭配”、HitTestInvisible、无布局诊断/越界。TryInteract=True后PromptText=Hidden；CloseTop返回True，移到范围外(-2100,-100,105)仍Hidden，再进入有效范围恢复文字。此为接口/运行树与视觉检查，未冒充物理E/Esc输入。
- PIE结束确认dirtyPackages为空，恢复临时后台CPU/Slate节流设置。统一入口`Package`：`Saved/T01/20261029-162019-Package-0a4c26/result.json`退出0，实际Win64编译、目标地图Cook/Stage/Package通过；`Saved/Cooked/Windows/Wizard/Content/Wizard/UI/WBP_InteractionPrompt.uasset`存在。对输出包执行`Smoke -SkillSmoke -PackageRoot 'Saved/Packages/20261029-162019-Package-0a4c26'`，`162311-Smoke-58aeec`六技能/四种取消/Alchemy/菜单暂停PASS，退出0、renderingEnabled=true、bridgeLoaded=false。
- Standards：按编码规范第2/4/6节，独立显示组件和蓝图布局分工、弱引用/委托解除、点击穿透、无交互轮询检查通过；实际构建、8项自动化、MCP资产编译保存、Cook/包和diff检查通过。Spec（R03/R04/R06及本次用户布局要求）：提示在交互物上方、随镜头定位、开菜单/离开范围隐藏和重入恢复均经PIE视觉/运行树验证；独立包回归通过。本轮未重测物理输入/拖拽，不改变其映射与路由。T03整体仍in-progress，未提交/推送。

### 通用事件交互提炼（2026-10-29）

- 用户指出交互应供后续商店复用并取消0.1秒交互检查。本轮新增Interaction模块：PlayerState持有候选与提示，目标查询球负责进出范围，原生委托接入业务。E调用TryInteract，HUD只订阅OnPromptChanged；原0.1秒刷新仅保留技能冷却数值。
- 事件来源：范围Begin/EndOverlap、Avatar位置变化/重绑/失效、目标变换/禁用/销毁、生命变化与菜单状态。初次绑定只读取Pawn当前重叠；不再通过SkillLoadout枚举全场Alchemy。多目标选择最近有效目标，距离相同用路径稳定排序；按E和提交搭配时再次校验。
- AlchemyStation只绑定营地条件、模型引用和OpenSkillMenu动作；技能搭配查询已注册Alchemy候选验证权限。Shop接入约定见docs/INTERACTION.md，不在本轮实现交易或更改T07状态。
- 首轮UHT/UBT`Saved/T01/20261029-154556-BuildEditor-1eb66e`通过。首次自动化启动因Sphere构造中SetSphereRadius触发匿名BodySetup默认子对象断言退出3，已改构造期InitSphereRadius；并非放宽测试。修复实际编译`155107-BuildEditor-008d4e`退出0。
- 全量自动化`Saved/T01/20261029-155112-Test-61a336/AutomationReport/index.json`：8 succeeded/0failed/0notRun，退出0。新增Wizard.Interaction.EventsAndLifetime通过真实碰撞重叠检验两种动作共用入口、最近选择、同目标提示去重、菜单抑制、死亡恢复、目标销毁/启用和范围内Avatar重绑；既有7项回归通过。
- 现有地图MCP只读确认AlchemyStation具备CampBounds与InteractionTarget。PIE传送到(-1412.9,-631.4,105)触发范围注册，TryInteract返回True并打开真实技能菜单；CloseTop后读回HUD提示“E　技能搭配”；立即传送回(-2100,-100,105)同一序列读回提示为空，TryInteract=False。此为真实地图接口/碰撞验证，不假称物理E键验收。没有改地图布局或输入映射资产。
- 停止PIE并确认无脏包后恢复后台CPU/Slate设置。统一入口Package：`Saved/T01/20261029-155433-Package-18491d/result.json`，实际Win64游戏目标编译及L_WizardClearing Cook/Stage/Package退出0；输出`Saved/Packages/20261029-155433-Package-18491d`。
- 对该包顺序运行`Smoke -SkillSmoke`与`Smoke -CombatSmoke`（均传入上述PackageRoot，无NullRHI）：`155751-Smoke-457266`六技能Montage/碰撞/GE、四种取消、Alchemy/交换/重复拒绝和菜单暂停PASS，`155814-Smoke-5486a9`普通攻击回归PASS；两轮退出0、renderingEnabled=true、bridgeLoaded=false。可复跑命令为`pwsh -NoProfile -File Tools/Build/Invoke-Wizard.ps1 -Action Smoke -SkillSmoke -PackageRoot 'Saved/Packages/20261029-155433-Package-18491d'`（普通攻击换用`-CombatSmoke`）。
- Standards：按编码规范第2/4/6节检查职责边界、弱引用/委托解除、无交互Tick/定时器及公开接口测试；UHT/UBT、8项自动化、diff检查、Cook/Windows包与成品回归通过。Spec（R01/R03/R04/R06）：原范围/遮挡/营地限制及暂停/提交失败路径保持，事件提示在真实地图验证通过；本轮未重新注入物理E/Esc或拖拽输入，接口/成品自动化不冒充人工输入验收。独立包包含之前HUD冷却/法力提示修改；T03整体美术/音效打磨仍in-progress，T07不提前完成。未提交/推送。

技能数值与表现参数是内容初值；营地区域判定和搭配状态的公开接口在本票据落定，供 T08/T09 复用。讲解能力生命周期、GE、Tags 与输入槽解析。

### HUD冷却显示调整（2026-10-29）

- 用户反馈功能基本正常，UI/技能粒子/音效仍待改进；本步按指定截图先调整HUD：去掉“就绪”，将倒计时叠在技能图标中央，仅冷却期间显示，保留技能名称/槽位数字及既有法力不足提示。
- C++：SkillTile新增可选CooldownText绑定与SetCooldown显示接缝；Configure清空倒计时，HUD创建时立即初始化实际GE剩余时间，原0.1秒刷新继续读取真实冷却。搭配页面不调用HUD冷却显示，不改变其拖拽文案。
- 资产全部通过UE-MCP：保留现有IconSize/Image，加入IconOverlay与居中CooldownText，默认Collapsed、字号30、描边2、HitTestInvisible；Image在Overlay中横纵Fill，保证原图标尺寸不因重排缩小。没有新增图标/软引用或修改地图。
- UHT/UBT：`Saved/T01/20261029-152745-BuildEditor-3606cf`，实际编译/链接退出0，非LiveCoding/干净重编译。`Test -TestFilter Wizard.UI`：`Saved/T01/20261029-152822-Test-900c4a/AutomationReport/index.json`，1 succeeded/0failed/0notRun，退出0；此为已有UI生命周期回归，不把它当作新增视觉检查。
- PIE：截图`Saved/T03/HUDCooldown/cooling.png`可见火球0.2、冰箭2.3分别覆盖各自图标中央；下方仅槽号，没有“就绪”。冷却结束读回CooldownText.visibility=Collapsed/isVisible=false；`ready-after.png`记录结束状态。临时将世界速度降为0.05用于观察后恢复1，后台CPU/Slate节流在停止PIE后恢复原值。
- Standards：本步编译/UI回归/MCP资产编译保存和diff检查通过；业务显示由C++管理，布局由Widget Blueprint配置。Spec：本次HUD显示要求经PIE截图与运行树读回通过，未改施法/消耗/冷却规则。未重复Cook/独立包：本步无新资源引用，旧独立包不含此修改；T03整体美术/音效打磨继续in-progress。未提交/推送，保留用户已有改动。

#### 同轮追加：法力不足置于图标中央

- 用户追加要求：法力不足也放在图标中央。接缝更新为SetHUDState，显示优先级为“正冷却倒计时 > 法力不足 > 就绪隐藏”；状态下方始终只显示槽位数字。搭配菜单原文案保留。
- 复用图标状态叠层，法力不足按“法力/不足”两行显示。通过MCP将状态层字号最终调为20、保留描边2及居中，避免两行提示超出78×78图标；字号30的前一版已被本次布局修正取代。
- 实际UHT/UBT编译`Saved/T01/20261029-153243-BuildEditor-e62cb4`退出0。未重复与此显示分支无关的玩家状态属性测试；本轮前一步Wizard.UI测试1/0/0保持记录，不宣称它覆盖追加分支。
- PIE通过GAS瞬时Mana GE将法力扣至0，截图`Saved/T03/HUDCooldown/no-mana-fit.png`与运行树确认四图标中央均显示提示，Overlay子文本未超父边界（extendsOutsideParentBounds=false）。恢复法力、真实ActivateSlot成功后再次扣空，`cooldown-priority.png`验证冷却优先，其他技能显示法力不足；就绪隐藏行为沿用本步前一轮运行树验证。
- 验证用法力仅改PIE运行实例，未写定义/CDO；临时时间缩放恢复1，停止PIE后恢复后台CPU/Slate节流，资源无脏包。Standards：编译、MCP资产保存与diff检查通过；Spec：本次追加显示要求通过PIE实际预览。未重打独立包，T03整体仍in-progress，未提交/推送。

## 用户确认的实施方案（2026-10-29）

- 不使用 ArrowTrail。火球使用 Free_Magic 的 Projectile1_Base/Hit1；冰箭复制 Projectile2_Base/Hit2 改冰系颜色，参考 Area1 配色但不增加范围伤害。项目副本避免影响现有普攻。
- 六技能开局解锁；初始四槽火球、冰箭、雷击、闪现。1234 Started 每次施放一次，不长按续发；有效技能打断普攻并清除连射，无效请求不打断。
- 营地靠近现有 Camp_Alchemy 按 E 打开页面；交互默认 250cm 并检查遮挡。页面暂停世界，复用 Menu 层与 CommonUI 返回。
- 候选拖入槽位立即替换；重复装备拒绝，槽位间拖拽交换；取消和无效放下不修改状态。每次提交重新校验营地、交互距离、存活及对象有效性。
- 冷却跟随技能而非槽位。施法期间拒绝其他主动技能；普通松开允许完成，输入取消/UI/死亡/Avatar失效清理未完成施法。提交前失败不扣资源，提交后取消不退款。
- 护盾不叠加，再次成功施放刷新吸收量和时长；满血治疗失败，不扣资源。闪现检查沿途阻挡、地面和胶囊空间，不穿墙。
- 搭配属于 PlayerState，提供快照/恢复与有效变更事件，磁盘保存留 T08；完整挑战与死亡回营地留 T09。
- T02 前置验收收尾后进入实现；每步记录 Standards/Spec 与验证边界。全部资产操作通过 UE-MCP。


### Review修复与炼金台MVC重构（2026-10-29）

- 用户授权：先修复雷击敌人互相遮挡，再将炼金台页面重构为MVC并删除旧实现；闪现施法中移动超距问题本轮暂缓，未修改玩法规格。
- 雷击：范围结算收敛到`WizardSkills::ApplyAreaDamage`，能力负责出手时序与Cue；Overlap先收集目标，遮挡查询统一忽略全部敌人，场景Visibility障碍仍生效，每个存活敌人只结算一次。旧Ability内结算块已移除。
- MVC：Model继续由`UWizardSkillLoadoutComponent`唯一持有搭配状态/营地规则/事务；新增`UWizardSkillMenuController`负责页面会话、拖拽身份校验、候选/槽位显示数据、命令派发及模型事件订阅；`UWizardSkillMenu`和Tile仅显示/转发交互。每次激活创建新Controller，关闭解绑并拒绝旧请求；未复制四槽状态。
- 清理：删除页面原HandleDrop/CanDragSkill、直接模型引用/事件句柄及Drag.SourceMenu；Tile不再反向调用页面业务。已搜索Source/Wizard/UI确认上述旧符号无残留，唯一拖拽业务实现位于Controller。保留现有反射类/BindWidget名称和布局，无资产迁移或兼容实现。
- UHT/UBT：`Saved/T01/20261029-173749-BuildEditor-1288d1`，实际反射生成、编译和链接通过；修复雷击后`Saved/T01/20261029-173955-BuildEditor-389c3a`再次实际编译链接通过，均退出0。
- 先复现后修复：`Test -TestFilter Wizard.Skills.AreaTargetsAndOcclusion`，`Saved/T01/20261029-173907-Test-f58b47`退出1，明确断言“非致死前排不遮挡后排范围伤害”预期950、实际1000，保留失败证据。修复后统一`Test`：`Saved/T01/20261029-174010-Test-3789c1/AutomationReport/index.json`，9 succeeded/0failed/0notRun，退出0。
- 新增真实碰撞/GAS用例验证非致死双目标均扣血一次、范围外不受伤、墙前受伤/墙后免伤；TargetsAndCampTransactions补充无Widget依赖的Controller公开接口验证：替换/交换/卸下/空槽显示、已装备禁拖、过期槽身份、跨页面会话、离营地提交失败原子性和反馈、关闭订阅清理。既有冷却、护盾、输入、UI生命周期用例保持。
- PIE：通过真实Alchemy交互接口打开页面，截图`Saved/T03/MVC/menu.png`确认六候选/四槽/灰显/布局。CUA实际拖拽治疗至第四槽、第一第二槽交换、第四槽拖回滚动区空白卸下、已装备火球候选拖向空槽拒绝；每步页面与HUD同步，最终截图`Saved/T03/MVC/drag-verified.png`。本轮物理E未稳定触发，开页证据是TryInteract接口，不冒充E验收；Esc属于编辑器停止快捷键，本轮不据此断言游戏返回键验收。
- PIE六技能Smoke：通过`KismetSystemLibrary.ExecuteConsoleCommand`将WorldContext指向PIE后执行`Wizard.SkillsSmokeTest`，`Saved/Logs/Wizard.log` UTC09:44:04～09:44:15六项Passed=1、四条取消SpawnDelta=0/NoRefund=1/Casting=0及WIZARD_SKILLS_SMOKE_PASS。摘录`Saved/T03/MVC/skills-smoke.log`。此前一次命令误发Editor世界报Player or catalog invalid，另一次在菜单暂停时启动报Busy，均为验证调用条件错误，日志保留；关闭菜单并指定PIE重验通过，未修改断言。
- 收尾：停止PIE，恢复临时后台CPU/Slate节流设置，MCP读回dirtyPackages为空；未修改用户地图、布局或配置资产。未Cook/打包，遵循减少频繁打包要求，旧独立包不含本轮C++变更；未提交/推送。
- Standards：按编码规范2/3/4/6节，唯一业务状态、Controller/View职责、弱会话引用、订阅关闭清理、GE结算、实际UHT/UBT及9项自动化通过；相关旧符号搜索与diff空白检查通过。
- Spec（R03～R08）：雷击多目标/墙体规则、搭配替换/交换/卸下/失败不提交/冷却保留与页面暂停通过自动化和PIE；拖拽关键路径有物理操作证据。独立包、本轮E/Esc完整输入及整体音画打磨仍未完成，T03维持in-progress，不以局部修复宣称整票完成。
