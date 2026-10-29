# T02 普攻战斗链路

- Status: done
- Blocked by: [T01](./01-environment-and-quality.md)
- Spec: R01、R02、R03、R04、R07、R08，正文见 [规格](../spec.md)
- Updated: 2026-10-29

## T03 开工前置复验（2026-10-29）

- 当前版本 Package：`Saved/T01/20261029-140325-Package-12a8e9/result.json`，退出0，实际编译Editor/Game目标并完成Cook/Stage/Package/Archive。成品为`Saved/Packages/20261029-140325-Package-12a8e9/Windows/`，包含最新三层UI与玩家生命/法力模块。
- 统一Test：`Saved/T01/20261029-140533-Test-291e8a/AutomationReport/index.json`，4 succeeded/0 failed/0 notRun，退出0。
- PIE初始观察到相机似乎在世界原点；实际组件读回为正确俯视位置，GetTimeSeconds一直0。临时关闭后台CPU与Slate节流后时间推进、截图恢复正常俯视；没有修改相机/地图资产。停止PIE后两个临时设置均恢复原值。不能将未推进模拟的初始帧当作相机资产缺陷。
- PIE实际Escape结束预览，受编辑器停止快捷键影响，不作为CommonUI游戏路由失败证据。已打开本轮独立包进行物理输入复验。
- CUA检测到用户正操作成品窗口，停止自动按键；等待用户报告WASD、单击/长按/松开、Esc开关与暂停输入隔离。日志`Saved/T02/t02-physical-input-20261029-visible.log`。此前隐藏启动的测试进程已停止，以可见实例验收为准。
- Standards：当前目标编译、4项自动化与成品打包通过；Spec：独立包HUD与俯视画面可见，真实按键完整矩阵及当前版本CombatSmoke仍待验证，整票保持in-progress。

## 目标与范围

在既有场景中完成移动、鼠标瞄准、普通远程攻击、最小敌人受伤与生命 HUD。以一条真实可运行链路验证 Enhanced Input → C++ → ASC → Ability → 命中 → GE → 属性 → HUD。

## 输入、输出与不变量

- 输入：移动轴、鼠标世界指向与普通攻击按下/持续/松开/取消。
- 输出：角色移动和朝向、投射物/命中、敌人生命变化与最小 HUD。
- 普攻独立于四槽、不耗法力；间隔来自配置。伤害权威状态只有一份。
- 输入绑定、ASC 初始化与能力授予在角色重建后不重复；异步任务和定时器有结束路径。

## 失败与边界

验证无效目标、连续输入、松开/取消、中断/死亡、重复初始化与角色重建。用最小可激活 CommonUI 菜单验证暂停、返回和世界输入隔离，为后续真实界面提供入口。

## 验收条件

- [x] Input Action/Mapping Context 资产通过增强输入子系统和 C++ 绑定真实生效，WASD 移动、鼠标瞄准。
- [x] 左键单击一次、长按连射、松开或取消停止，攻击间隔可配置。
- [x] 普攻经 GAS 授予、激活、提交和结束/取消，命中通过 GE 修改目标属性，伤害规则明确且可测试。
- [x] 最小敌人可受伤，生命 HUD 通过状态事件正确刷新。
- [x] 配置正确的 CommonUI viewport 与最小路由，验证菜单开关/暂停/返回及连射清理，解决既有路由配置告警。
- [x] 角色比例、法杖挂接、动作与出手位置在实际镜头中验证；不覆盖场景手工布局。
- [x] 构建、受影响自动化、实际输入与失败路径、独立包内的最小战斗链路均有证据。

### T02 收尾结论（2026-10-29）

- 用户在本轮成品实际试玩后回复“全部正常”，确认WASD、瞄准、单击/长按/松开、Esc开关暂停及暂停时点击/移动隔离。物理输入证据来自用户试玩，区别于业务接口测试。
- 当前成品CombatSmoke：`pwsh -NoProfile -File Tools/Build/Invoke-Wizard.ps1 -Action Smoke -CombatSmoke -PackageRoot Saved/Packages/20261029-140325-Package-12a8e9`，退出0，证据`Saved/T01/20261029-140813-Smoke-b92bf5/result.json`及`packaged-combat-smoke.log`，WIZARD_COMBAT_SMOKE_PASS，bridgeLoaded=false。覆盖真实Notify、GE/GC、连射、UI栈、死亡/中断/Avatar与任务清理。
- 本轮Package为实际Editor/Game编译、Cook 0 errors/0 warnings、Stage/Archive成功；统一Test 4/0/0。此前UIRoot整轮Smoke未通过记录已由此当前成品回归覆盖，不删除历史失败证据。
- Standards：通过，依据本机UE5.4契约、项目C++/Blueprint边界、GAS生命周期与MCP资产流程；Spec：T02范围R01/R02/R03/R04/R07/R08通过。角色与法杖沿用用户调整后的表现，已在成品镜头可检查；未覆盖T03技能或后续完整挑战/存档。
- T02转done，解除T03前置依赖。保留性能历史问题的适用边界：后台节流与未推进模拟不计玩法缺陷，未宣称长期性能或驱动根因已解决。学习资料沿用票据内路径，用户掌握程度未测试；本轮未提交/推送。

## 验证记录

未执行本票据功能验收。已有模板移动或反射调用只属于早期基准。
Standards：未执行。Spec：未执行。

## 未决实现与学习

ASC Owner/Avatar 归属、初版伤害公式与角色动画接线在任务内落定；公式按规格中的待审阅标识处理。讲解输入到 HUD 的实际调用路径，材料留在本地。

### 本轮实现约束（2026-10-28）

- 用户补充：动画蓝图继续采用 Blueprint；C++ 提供角色和战斗状态，动画状态机、混合与动画接线留在动画蓝图。
- 前置 T01 已 done。本轮在保留现有关卡手工布局与已有工作区改动的前提下推进。


### 已执行证据（2026-10-28，本轮）

- `BuildEditor`：`Saved/T01/20261028-125411-BuildEditor-0eee44` 与修复测试后的 `20261028-125705-BuildEditor-679e44`，均退出 0，实际编译并链接；不是干净重编译。
- 资产装配：`Tools/Combat/setup_combat_assets.py`；`Saved/T02/setup-assets-retry.log` 退出 0，生成 IA_Move/IA_Attack/IA_Menu、IMC_Combat、CommonUI 返回配置、WBP_CombatHUD/WBP_PauseMenu、DA_BasicCombat；地图只增 T02_CombatTarget，没有重跑布景脚本。首次生成时 CDO 缺资产提示属装配前状态，后续新实例 Cook 可加载。
- `Package`：`Saved/T01/20261028-125601-Package-a82db9`，退出 0，游戏与编辑器目标编译、Cook/Stage/Package/Archive 完成。Cook 0 error、1 warning（GameplayCue 扫描路径未缩小）；成品位于 `Saved/Packages/20261028-125601-Package-a82db9/Windows/`。
- 独立包业务验收：`Smoke -CombatSmoke -PackageRoot 'Saved/Packages/20261028-125601-Package-a82db9'`，`Saved/T01/20261028-125751-Smoke-dae84b/packaged-combat-smoke.log`，退出 0，WIZARD_COMBAT_SMOKE_PASS。真实投射物/GE/HUD、无耗蓝、松开停止、菜单业务接口暂停/恢复与换 Avatar/幂等授予通过。0.35 秒间隔在 1.656 秒窗口命中 5 次，最低要求 4 次通过。该测试调用公开业务接口，不模拟物理按键或 CommonUI 返回路由。
- 失败与修复：首次构建 Controller 局部变量遮蔽与缺头文件已修复；首次启动 GE 构造 AddComponent 匿名 NewObject 断言，改为命名默认子对象后已实际启动/打包/战斗通过。首次自动化临时世界重复初始化已修复；第二次临时世界缺组件注册导致 AttributeSet 不可用，正在修正夹具并重验，不计为通过。
- 真实操作：已打开普通成品窗口，HUD/现有法师角色/目标可见；Windows 安全中心网络授权弹窗挡住操作，已请求用户点击取消；不需要为单机游戏开放网络。物理输入、长按、Esc 返回/焦点与视觉验收保持待验证。

### 双轴阶段结果

Standards：核心源码静态检查、UHT/UBT、独立包业务生命周期通过；自动化夹具重验及真实输入/路由仍待完成，不能判整票通过。详见 `docs/planning/t02-review-2026-10-28.md`。

Spec：最小业务链路在独立包通过；R01/R02 的物理操作、R03/R04 的 CommonUI 实际路由及法师动作/法杖/出手位置仍待实际验收。T02 保持 in-progress。

工程取舍：玩家 ASC 归 PlayerState，Character 为 Avatar；敌人 ASC self/self。普攻每发激活、提交、结束，冷却标签移除且仍按住时续发。初版伤害规则为正攻击力时 max(AttackPower - max(Defense, 0), 1)，零/非法攻击力不伤害；作为待试玩平衡初值，不改变规格。

学习资料已准备在 `.local-learning/lessons/2026-10-28-basic-combat.md` 与 `.local-learning/assessments/2026-10-28-basic-combat.md`；用户掌握度未测试。未提交、未推送，保留既有工作区改动。

### 最终自动化重跑（2026-10-28）

`BuildEditor`：`Saved/T01/20261028-130035-BuildEditor-627728`，退出 0，实际重新编译测试夹具并链接。`Test`：`Saved/T01/20261028-130102-Test-1ee602/AutomationReport/index.json`，退出 0；Wizard.Combat.DamageEffectAndClamps、Wizard.Environment.SceneStructure、Wizard.Environment.StartupConfiguration 三项均 Success，0 failed、0 notRun。真实 ASC/GE 攻防、法力不变、致命伤害下限、最大生命下降夹取及上限恢复不治疗均通过。此前自动化夹具失败已修复并重验。

Standards 当前结果：受影响源码/UHT/UBT、三项自动化、独立包业务检查及 git diff --check 通过。Spec 当前结果：业务链路已通过，物理输入/焦点/CommonUI 返回及动画表现仍待验收，整票继续 in-progress。最新包与最终编辑器源码的差异只有测试夹具（WITH_EDITOR），运行时业务相同。当前游戏普通试玩窗口保留供用户取消网络提示后继续实际验收。

### 2026-10-28 分步修正：GameMode 子蓝图配置玩家类

- 用户要求：DefaultPawnClass 的具体角色资产由 GameMode 子蓝图配置，原生 C++ 不写死玩家蓝图路径。本次保持现有角色选择，不包含法师美术、动画或出手点改造。
- 实现：`AWizardGameMode` 移除角色 FClassFinder 与相关 include；新增 `/Game/Wizard/Blueprints/BP_WizardGameMode`，其 Class Defaults.DefaultPawnClass 初始配置为现有 BP_TopDownCharacter。项目 GlobalDefaultGameMode 与 L_WizardClearing 的 GameMode override 同时接入该子蓝图。原生 Controller/PlayerState 默认保持原样。
- 装配入口：`Tools/Combat/setup_game_mode.py`，首次创建时设置角色；已有 GameMode 蓝图不覆盖其默认值。只更新项目对应配置项和关卡 WorldSettings，不重新生成布景或改动场景演员。运行前通过编辑器保存全部资产并正常关闭。
- BuildEditor：`Saved/T01/20261028-181506-BuildEditor-1cce30`，退出 0，实际编译 GameMode 和受影响启动测试并链接。
- 资产装配：`Saved/T02/GameModeBlueprint/setup.log`，退出 0，WIZARD_GAME_MODE_SETUP_PASS；首次创建前缺包提示属于新资产尚不存在，后续新实例测试可加载。
- 环境回归：`Test -TestFilter Wizard.Environment`，`Saved/T01/20261028-181621-Test-1fae81/AutomationReport/index.json`，2 Success、0 failed、0 notRun，退出 0。覆盖原生 GameMode 不指定角色 BP、项目使用 GM 子蓝图、地图 override 与项目一致、有效 Pawn/Controller/PlayerState 类型正确及既有场景结构。
- Standards：本次原生行为与蓝图资源配置边界符合用户要求；当前构建、环境回归和 diff 检查通过。Spec：本步配置链路通过，整项 T02 仍为 in-progress，其他待验收项没有随本次更改判完成。

### 2026-10-28 UE-MCP 连接修复与配置重新验收

- 用户明确要求：蓝图、资产及其配置修改必须使用 UE-MCP，连接失败先修复，不允许脚本或界面绕过。已同步编码规范；本轮新增的 `Tools/Combat/setup_game_mode.py` 已移除，上节装配记录保留为历史事实，不再是可用入口。
- 根因：Windows TCP 排除范围 61159～61258 与 61259～61358 覆盖派生端口 61217 及全部 50 个探测偏移。原端口的实际 Loopback Socket.Bind 在 61217/61267 均返回 NativeErrorCode=10013；18777 同样独占绑定成功。不是通过关闭防火墙解决。
- 修复：本机忽略配置 `ue-mcp.local.yml` 设置 `ue-mcp.bridge.port: 18777`；通过 MCP project.set_project 重新加载目标配置，editor.start_editor 成功（28.1 秒）。Saved/UE_MCP_Bridge/port.json 为 listening，日志确认仅监听 ws://127.0.0.1:18777。project.get_status 返回 mode=live、editorConnected=true。
- MCP 实际读写：blueprint.compare_class_defaults 核对父子类，只有 DefaultPawnClass 覆盖；blueprint.set_class_default 写现有 BP_TopDownCharacter；asset.save(force=true) 成功，BP_WizardGameMode 文件 20955 bytes；level.set_world_settings + level.save 成功（savedCount=1, failedCount=0）；project.set_config 写项目 GameMode。随后经 MCP 再读默认值和关卡设置一致。
- MCP PIE 验证：editor.play_in_editor 启动；level.get_actors_by_class 在 PIE 中返回 BP_WizardGameMode_C_0；editor.get_pie_pawn 返回 BP_TopDownCharacter_C_0，位置(-2100,-100,103.884)。已请求停止 PIE。实际使用的是 GameMode 子蓝图，C++ 不再选择角色资源。
- 已启动的 Package 在新约束到达前进入 Cook，随后后台完成（Saved/T01/20261028-181654-Package-13f51f，退出0）；约束到达后没有再用外部脚本或界面修改蓝图/资产。后续本步验证以 MCP 读写与 PIE 为准，未另外运行本轮独立包 Smoke，不声称它覆盖 MCP 保存后的资产版本。
- Standards：本步 C++ 构建、环境测试、MCP 实际读写保存与 PIE 通过；端口配置本机化，未改变系统保留范围或防火墙。Spec：仅完成玩家类选择移至 GM 子蓝图；现有玩家、美术、动画、出手点均未扩展，T02 整票保持 in-progress。

### 2026-10-28 分步创建玩家角色蓝图

- 用户要求：先创建玩家角色蓝图。本步由 subagent 全程使用 UE-MCP 创建 `/Game/Wizard/Blueprints/BP_WizardCharacter`，直接继承 `/Script/Wizard.WizardCharacter`；原生类继续提供组件和业务，蓝图留作资源装配入口。
- UE-MCP 验证：blueprint.create 返回直接父类 WizardCharacter；玩家与 GameMode 蓝图 compile 均 0 errors、0 warnings；asset.save 确认已落盘且不再脏；blueprint.read 与 get_cdo_properties 确认父类和 DefaultPawnClass。
- `BP_WizardGameMode.DefaultPawnClass` 已通过 MCP 改为 `BP_WizardCharacter_C`。MCP PIE start/status 确认 isPlaying=true；get_pie_pawn 返回 BP_WizardCharacter_C_0，完整类路径 `/Game/Wizard/Blueprints/BP_WizardCharacter.BP_WizardCharacter_C`，出生位置(-2100,-100,103.884)。已停止 PIE 并打开新蓝图供查看。
- Standards：本步创建/配置/编译/保存/运行均使用 UE-MCP，无外部资产装配脚本，未改 C++ 或地图布局。Spec：新玩家蓝图及 GameMode 选择在 PIE 生效；尚未装配 Mesh、动画蓝图、法杖或出手定位，当前为无模型角色骨架，不以场景现有法师摆件代替玩家表现验收。
- 本步未改 C++，未重跑 UBT；未执行 Cook/打包，不声称独立包已包含本步新资产。整票 T02 仍 in-progress，未提交/推送。

### 2026-10-28 BattleWizardPBR 模型与动画适配（进行中）

- 本步范围：新玩家 BP_WizardCharacter 配置原包 WizardSM、同骨骼动画蓝图、待机/前后左右移动混合及普通攻击表现。动画状态/混合/资源引用仍在 Blueprint，蓝图资产修改只使用 UE-MCP，不重跑布景脚本，不改地图布局，不在本步改 MuzzleOffset 伤害/出手规则。
- C++ 表现接缝：AWizardCharacter 新增 BlueprintImplementableEvent OnBasicAttack；BasicAttackAbility 仅在成功生成有效弹丸后通知角色蓝图。事件本身不结算伤害，不影响原有 Commit、GE、冷却或停止输入路径。
- 完整构建遇到 UE-MCP 新源码的 UE5.4 兼容问题，失败日志为 Saved/T01/20261028-183037-BuildEditor-8a0d11（缺头文件）、183318-BuildEditor-f7c24c（PCG API不存在）、183611-BuildEditor-0c9927（未导出测试类符号）。最小兼容调整及限制见 docs/planning/t02-review-2026-10-28.md；没有通过绕过 MCP 修改资产解决。
- 修复后 BuildEditor：Saved/T01/20261028-183754-BuildEditor-08b9f8，退出0，实际链接 Wizard 与 UE_MCP_Bridge；UE-MCP editor.start_editor 成功，ready 34.4秒。资产配置随后由 subagent 经 MCP 继续执行。
- 只读资源核实：MCP asset.get_mesh_info 返回 WizardSM 高1.970589m，3927顶点，原包有效材质，无fallback；Skeleton=/Game/BattleWizardPBR/Meshes/UE4_Mannequin_Skeleton，WeaponSocket在Weapon骨。原包动画与其同骨骼，无需重定向。

#### 模型与移动动画的首轮 PIE 证据

- 玩家实际类为 BP_WizardCharacter_C，Mesh 为 /Game/BattleWizardPBR/Meshes/WizardSM，AnimationBlueprint 模式，运行实例为 ABP_WizardCharacter_C。组件Mesh相对Z=-96、Yaw=-90；地面站立时组件世界Z约7.884，角色中心Z约103.884，继承胶囊与模型比例经正面检查。
- ABP为Blueprint，变量 VelocityRight/VelocityForward 来自 Pawn 速度按 ActorRotation 反旋转后的局部Y/X；TryGetPawnOwner无效时有归零分支。BS中心Idle01、前后左右BattleWalk；当前初值MaxWalkSpeed=300，匹配BS坐标范围，不新增跑步输入。
- 通过MCP临时关闭本轮PIE Controller Tick固定Yaw0，导航移动时前进Forward约+300、后退约-300、右行Right约+299.31（Forward-20.36由避障产生）、左行加速中Right-102.40；停止后两值归0，Controller原Tick已恢复。导航接口不是物理WASD验收，不能据此勾选整项R01。
- 右行过程中连续实际骨姿态：foot_l组件位置由(22.14,34.52,12.15)变为(16.17,34.15,18.31)，运行ABP/源骨骼均正确，证明不是只改模型或静态Pose。首次前进撞到测试靶后采到Idle，改为持续侧移后得到动态证据，未因此误改资产。
- Saved/T02/Animation/idle-front.png 为MCP近景截图；瞬时SceneCapture缺少实际viewport的Lumen历史，截图明显偏暗，不能拿该图单独判定材质或最终镜头效果。法杖与攻击动作接入完成后继续实际viewport验收。

#### 模型、动画与法杖最终编辑器验收

- 最终资产：BP_WizardCharacter、ABP_WizardCharacter、BS_WizardLocomotion、AM_WizardBasicAttack；全部通过 UE-MCP 创建/配置/编译/保存。两个BP最终均0错误0告警；四个包已落盘且notDirty。
- 最终Idle使用原包Idle02（替换首轮Idle01，更放松的握持姿态）；四方向仍为BattleWalk。AnimGraph缓存移动姿势，用两个独立UseCachedPose分别接Slot.Source与Layer.BasePose，spine_01+Weapon骨支路播放上身攻击，下身保持移动。修复了复用同一Pose输出导致旧链接被自动断开的实际问题，Source visible but ignored信息已消失。
- 原包Staff03SM作为MagicStaff组件；单写组件模板AttachSocketName不能配置SCS节点socket，首轮实际实例读回None，已改为角色蓝图ConstructionScript显式AttachToComponent。最终真实实例读回AttachParent=CharacterMesh0、AttachSocketName=WeaponSocket；参考旋转(-90,-19.471241,19.471148)。正面与侧面握持检查通过，未据模板值冒充实例验证。
- Native StartBasicAttack → GAS实际生成弹丸 → OnBasicAttack Blueprint事件 → PlayAnimMontage集成验证：持续攻击期间真实弹丸存在（例如WizardProjectile62～64），GetCurrentActiveMontage读到AM_WizardBasicAttack；帧间会返回None属于每发动作结束后的空隙。停止输入后最终Montage=None，两局部速度归0。
- 边走边攻击：Right约299.31、Forward约-20.36，连续foot_l由(15.61,34.12,18.78)变(7.57,33.69,9.67)，foot_r由(-25.28,-18.71,9.88)变(-16.76,-18.38,19.20)；hand_r与Weapon同时保持动态，腿部没有被攻击层锁死。
- 实际PIE viewport截图：Saved/T02/Animation/final-idle.png（正常游戏镜头）、final-grip-front.png与final-grip-side.png（临时PIE相机近景），attack-slow-preview.png（仅为观察动作使用0.15时间缩放，不代表正常速度录屏）。这些截图由MCP capture_screenshot生成。近景相机/Controller Tick/时间缩放只改了PIE实例，已恢复800cm/-60度相机、Tick=true和时间缩放1，再停止PIE；没有改地图或持久相机布局。
- 攻击初值：Attack01原长1秒，Montage RateScale=3.3，单次约0.303秒，BlendIn0.03/Out0.04，以配合当前0.35秒间隔；未改攻击间隔、伤害、消耗或MuzzleOffset。这是动作节奏原型，不代表完成出手帧/发射位置同步或最终脚滑润色。
- 回归：UE-MCP editor.run_automation_tests(filter=Wizard.,maxTests=10) 返回ran3/passed3/failed0/abandoned0，三项均0错误0告警，覆盖GAS规则、启动配置与场景结构。git diff --check通过，仅CRLF提示。
- Standards阶段结论：角色表现资源与动画图在Blueprint，C++只发成功攻击表现事件；全程MCP资产操作，编辑器编译/保存/PIE/三项回归通过。Spec阶段结论：本步模型、待机、四方向移动、法杖握持、普通攻击与移动攻击表现已验；R01物理输入全链、出手点改造及其他T02待办仍未由本步替代验收。

#### 本轮 Windows 交付验证

- 通过MCP确认四资产notDirty、无脏地图后请求正常关闭编辑器。第一次Package紧接异步关闭触发进程仍在的保护检查，被正确拒绝；等待本项目进程退出后重跑，没有强制杀进程或跳过保护。
- `Package`：Saved/T01/20261028-190547-Package-221199/result.json，退出0；实际重新构建Game目标（含OnBasicAttack反射事件），Build/Cook/Stage/Package/Archive均完成。Cook 0 error(s)、1 warning(s)，警告为既有GameplayCueNotifyPaths未限定；UFS清单含ABP_WizardCharacter、BS_WizardLocomotion、AM_WizardBasicAttack及BP_WizardCharacter。
- `Smoke -CombatSmoke -PackageRoot 'Saved/Packages/20261028-190547-Package-221199'`：Saved/T01/20261028-190818-Smoke-b1881d，退出0，WIZARD_COMBAT_SMOKE_PASS；新玩家蓝图在独立包中加载并通过真实弹丸/GE/HUD/连射/暂停恢复/Avatar替换业务检查。该Smoke不对动画观感作像素断言，动画与握持效果由前述PIE多视角与运行pose证据承担，不宣称物理键鼠全量验收。
- 本步Standards通过：UHT/UBT、三项自动化、MCP蓝图编译/保存/运行读回、目标地图Cook与独立包业务Smoke均有证据。Spec本步模型/待机/四向移动/法杖/普攻表现通过；出手点与动作发射帧同步、最终脚滑/动作节奏和整票剩余项仍待后续，本票保持in-progress。
- 成品：Saved/Packages/20261028-190547-Package-221199/Windows/。学习说明已更新本地lessons，用户独立掌握未测试。未提交/推送，未改变用户场景布局。

### 2026-10-28 普攻 GE/GC 蓝图配置迁移（进行中）

- 用户认可当前动画，本步不调整动画/模型/法杖/出手点。目标是将GE与GC的配置留在Blueprint，能力行为与可测试伤害公式保留C++；此前没有已接入的GC，本次新增命中GC，不把它描述为已有功能迁移。
- C++删除仅配置的UWizardDamageEffect与UWizardAttackCooldownEffect；保留UWizardDamageExecution、CalculateDamage和native冷却语义tag。GA的DamageEffectClass与内建CooldownGameplayEffectClass由数据用GA子蓝图提供；DA_BasicCombat配置BasicAttackAbilityClass，PlayerState校验后幂等授予，缺配置不会回退硬编码能力。
- 冷却时间唯一权威移到GE_BasicAttackCooldown.DurationMagnitude，初值0.35；删除DA.AttackInterval与native ApplyCooldown覆盖，避免GE里可编辑的持续时间实际上被外部覆盖。连射仍由冷却tag归零事件续发，无第二定时器。当前只支持静态有限冷却>=0.05秒，普攻不得配置Cost。
- 编辑器完整构建：Saved/T01/20261028-195305-BuildEditor-461af6，退出0，UHT反射重新生成，实际编译/链接，无LiveCoding替代。
- 全部资产由subagent通过UE-MCP创建/配置/编译/保存：/Game/Wizard/GAS/Effects/GE_BasicAttackDamage（GameplayEffect父类，Instant，CalculationClass=WizardDamageExecution）、GE_BasicAttackCooldown（GameplayEffect父类，HasDuration .35与Cooldown.Wizard.BasicAttack标签）；/Game/Wizard/GAS/Abilities/GA_BasicAttack（原生业务父类，配置GE引用）；/Game/Wizard/GAS/Cues/GC_BasicAttackImpact（GameplayCueNotify_Burst父类，纯数据BurstParticles，FX_Cursor、DoNotAttach、缩放.5）。DA_BasicCombat连到GA子蓝图。最终实际路径以MCP读回为准。
- GC标签GameplayCue.BasicAttack.Impact显式配置，GE负责触发；DefaultGame.ini通过MCP限定GameplayCueNotifyPaths到/Game/Wizard/GAS/Cues。命中上下文仍由弹丸AddHitResult传给Spec，Burst根据ImpactPoint/ImpactNormal放置，不负责伤害。
- MCP editor.run_automation_tests(Wizard.)：3 passed、0 failed、0 abandoned，每项0错误0告警；伤害用例已加载真实蓝图GE，仍验证攻防、法力不变与生命夹取，并覆盖缺配置拒绝。
- MCP gas.validate_cue_coverage：1个GE cue link，exact匹配GC_BasicAttackImpact，0 uncovered；manager runtime/editor均1。工具宽扫的native基类与SKEL无tag条目不属于本项目GC缺失；并未将静态覆盖替代实际播放验收。正在补真实命中、粒子自行完成与Cue被抑制时伤害仍成立的Smoke证据。

#### GE/GC迁移最终验证（2026-10-28）

- MCP保存确认5包notDirty后正常关闭编辑器。`Package`：Saved/T01/20261028-200036-Package-398ea2，退出0，编辑器/Game目标含新Smoke实际编译链接、Cook/Stage/Package/Archive均完成；Cook 0 error(s)、0 warning(s)。UFS清单含GC_BasicAttackImpact与其余新GE/GA资源，Cue软发现目录在成品可用。
- 独立包`Smoke -CombatSmoke`：Saved/T01/20261028-200224-Smoke-1c29ec/packaged-combat-smoke.log，退出0。WIZARD_COMBAT_CUE: SingleDamage=25、ActiveImpactPeak=1；等待活跃粒子自然归零后抑制目标运行ASC的Cue，再单发WIZARD_COMBAT_CUE_SUPPRESSED: Damage=25、ExpectedDamage=25、ActiveImpactPeak=0。恢复原抑制状态后继续原有链路；0.35秒冷却在1.656秒命中5次，最低4次通过，最终WIZARD_COMBAT_SMOKE_PASS。
- 测试限定当前世界、FX_Cursor资源与NiagaraComponent.IsActive，允许池化组件对象留存；没有通过强制销毁粒子伪造生命周期通过。每个等待上限15秒、总90秒；所有失败/成功清理路径均恢复bSuppressGameplayCues。该验证说明“表现被关闭时不阻断伤害”，不把所有未知GC故障都宣称已穷尽。
- Standards：GE配置、GA资源组装、GC表现采用蓝图；C++保留授予/激活/提交/结束、弹丸命中和Execution。Blueprint/CDO只读，运行数据在Spec/Context，间隔只有冷却GE一份。构建、3项真实BP GE回归、Cue覆盖与独立包GC/伤害独立性检查通过。最终 git diff --check 报告 DefaultEditor.ini 与 DefaultGame.ini 文件末尾空行（UE/MCP 序列化生成）；未用外部工具改写配置绕过 MCP，这两项格式告警保留，本步源码未发现尾空格。
- Spec：本步保留单击/长按/不耗蓝和伤害规则，25攻击对0防御结算25；射速直接由冷却GE Duration配置，无隐藏DA覆盖。动画/模型和MuzzleOffset未更改，角色发射点与动画帧同步仍属于后续待办，T02整票继续in-progress。未提交、未推送。

### 2026-10-28 伤害 Execution 文件重命名与讲解

- 用户要求：文件名与UWizardDamageExecution对应。WizardCombatEffects.h/.cpp已改为WizardDamageExecution.h/.cpp，generated.h同步；所有Source旧include已清除。无关冷却tag/API移到WizardCombatTags.h/.cpp，GA/PS引用更新；类名、/Script/Wizard.WizardDamageExecution反射路径、算法及运行时行为不变。
- 开始验证时MCP发现GE_BasicAttackDamage有未保存改动，先保存该包以保留用户内容，再requireClean正常关闭；一次等待关闭的前置检查超时，没有强杀编辑器，实际退出后继续构建。
- BuildEditor：Saved/T01/20261028-202405-BuildEditor-467e38，退出0，UHT重新生成，实际编译新Execution/Tags文件并链接。MCP重新启动编辑器成功；editor.run_automation_tests(Wizard.) 3 passed、0 failed、0 abandoned，各0错误0告警。GE Executions读回仍为/Script/Wizard.WizardDamageExecution，不需要资产重绑或CoreRedirect。
- Standards：对应头先include、generated头匹配新文件名、伤害与标签职责分离，源码范围git diff --check通过；Spec：纯命名/职责归档，真实GE伤害测试仍通过，无玩法变化。本步没有重跑Cook/成品包，仍以此前相应范围的成品证据为准。
- 讲解依据：本机UE5.4 GameplayEffectExecutionCalculation.h中宏原定义与BlueprintNativeEvent契约；涵盖宏展开、捕获描述/快照、静态只读helper、RelevantAttributesToCapture、标签评估、输入/输出引用及IncomingDamage消费链。个人学习笔记位于.local-learning/lessons/2026-10-28-damage-execution.md，用户独立掌握未测试。

### 2026-10-28 可配置 ArrowTrail 普攻投射物（本步完成）

- 用户明确要求：替换原普攻球体为ArrowTrail粒子，资源与表现可配置，不写死C++。Execution属性捕获尚未理解已记录.local-learning/learning-plan.md待补讲，未标为掌握，当前继续工程开发。
- 当前旧实现由Actor持Sphere碰撞与ProjectileMovement，Native构造硬引用引擎球体网格，OnHit携带GE Spec结算一次并销毁；本步移除球体网格硬引用，保留移动/碰撞/伤害/GC行为。
- C++增加可配置Niagara组件Trail，BeginPlay激活，EndPlay立即停止，组件跟随Actor销毁，不分离无限循环拖尾。AWizardProjectile Blueprintable；GA新增ProjectileClass，实际Spawn使用配置类；缺失/抽象/过期类拒绝。速度与弹丸寿命仍来自DA_BasicCombat唯一配置，不增加被覆盖的重复旋钮；未改出手点、动画、间隔或伤害公式。
- Smoke预期类与系统从实际GA.ProjectileClass和该类CDO的Trail组件读取，验证运行实例与活跃组件、生命周期残留；更换蓝图或ArrowTrail变体无需改业务或测试中的固定路径。
- BuildEditor：Saved/T01/20261028-213515-BuildEditor-eb8be1，退出0，UHT生成5处反射，实际编译链接；MCP冷启动编辑器ready15.7秒。随后由subagent经MCP创建BP_WizardBasicProjectile与GA引用，默认候选NS_ArrowTrail_Magic，保留原包资源。
- 只读资源核实：Magic为GPU Ribbon+Sprite，无Mesh长轴；User.Size/Length/Lifetime/Trail Size由蓝图构造图绑定可调变量。视觉粒子寿命与DA弹丸寿命区分，后续PIE确认大小/尾长与清理，再写最终结论。

#### 最终配置与运行证据

- 全部蓝图操作经UE-MCP：创建并配置`/Game/Wizard/Blueprints/BP_WizardBasicProjectile`，其Trail.Asset为`/Game/ArrowTrail/FX/NS_ArrowTrail_Magic.NS_ArrowTrail_Magic`，局部旋转0、缩放0.5；`GA_BasicAttack.ProjectileClass`引用该蓝图。两个BP编译均0错误0告警，保存0失败，正常关闭编辑器前dirtyContentCount=0、dirtyMapCount=0。原ArrowTrail资源、地图、角色和动画未在本步修改。
- 蓝图Class Defaults的Projectile|Visual分类提供TrailParticleLifetime=0.15、TrailSize=0.5、TrailLength=0.5、TrailWidth=1。ConstructionScript用变量getter向Trail的User.Lifetime/Size/Length/Trail Size写值；12条图链接读回正确，非固定常量。重启后MCP读回4默认值、Trail.Asset、RelativeScale3D和GA.ProjectileClass均匹配。其他Niagara系统若没有同名User接口，应在该蓝图调整绑定，不宣称这些参数名通用于所有系统。
- UE-MCP自动化`editor.run_automation_tests(filter=Wizard.,maxTests=10)`：3 passed、0 failed、0 abandoned，各0错误0告警。真实PIE由Native StartBasicAttack发射，运行实例为BP_WizardBasicProjectile_C；截图`Saved/T02/ArrowTrail/held-slow.png`可见细蓝白轨迹。此图仅在0.15时间缩放下观察表现，随后恢复1与Controller Tick=true再停止PIE；未改持久相机或地图。正常速度截图`held-normal.png`恰处发射间隙，不用它声称捕获了弹丸。
- 首轮PIE Smoke失败原因：目标距250cm，弹丸在约0.12秒内命中，低帧率下整个飞行发生于两次CoreTicker检查之间；实际伤害25、命中GC峰值1。修复测试为StartBasicAttack同步完成生成后立即观察实际类和活跃Trail，原断言与后续清理检查保留，没有为测试改生产行为或增加委托。`BuildEditor`重编Smoke并链接，`Saved/T01/20261028-214820-BuildEditor-73f689/result.json`退出0。
- `Package`：`Saved/T01/20261028-214939-Package-b2c818/result.json`退出0，Game目标实际UHT/UBT编译，目标地图Cook 0 error(s)、0 warning(s)，Build/Stage/Package/Archive成功。UFS清单包含BP_WizardBasicProjectile、GA_BasicAttack和NS_ArrowTrail_Magic。成品`Saved/Packages/20261028-214939-Package-b2c818/Windows/`。
- `Smoke -CombatSmoke -PackageRoot 'Saved/Packages/20261028-214939-Package-b2c818'`：`Saved/T01/20261028-215211-Smoke-2a13a1/packaged-combat-smoke.log`与result.json，退出0。ConfiguredClassObserved=1、ConfiguredSystemObserved=1、ActiveTrailPeak=1；单发伤害25/命中GC峰值1，抑制GC时伤害仍25/GC峰值0，1.654秒连射5次（最低4次），结束ActiveTrails=0，WIZARD_COMBAT_SMOKE_PASS。覆盖真实配置类/系统加载、伤害/HUD、松开、暂停取消与Avatar替换；不把反射业务调用当物理键鼠验收，也不把组件激活当GPU画面像素断言。
- Standards：小范围只读审查未发现阻断项；TSubclassOf校验在Commit前，deferred spawn先注入Spec/DA再完成构造，资源和变换留BP，EndPlay统一停止owned组件，硬引用依赖在Cook与独立包中验证。源码范围与本次工程说明diff-check通过；全局既有Config文件尾空行问题仍保留，未绕过MCP用文本工具修配置。
- Spec：R02/R07/R08本步可配置弹丸与ArrowTrail替换通过；既有碰撞/伤害/冷却机制保留。此步不完成整票R01物理输入、CommonUI实际路由、出手点Actor化或动作发射帧同步，T02仍in-progress。特效随Actor立即结束，未实现脱离后淡出。编辑器已重新打开投射物与GA供检查。
- 学习资料：`.local-learning/lessons/2026-10-28-configurable-projectile.md`说明三个配置入口、粒子寿命与弹丸寿命区别；Execution捕获待补讲保留，用户独立掌握未测试。未提交或推送。

### 2026-10-28 PIE 启动严重卡顿（用户重启后恢复，暂停排查）

- 用户反馈每次点击Play进入游戏异常卡断，未攻击也可复现。上一步功能Smoke只证明业务与资源加载，不代表性能通过；本次重新检查启动表现。
- 复现与度量：UE-MCP控制CsvProfile STARTFILE/START → Play → STOP，原生FrameTime读取真实平台时钟。`Tools/Performance/Measure-WizardCsv.ps1`严格读取CSV数据与最终表头，输出长帧和来源行；尾部UE metadata单独处理，不输出其中环境标识。原始证据位于Saved/Profiling/CSV，摘要Saved/T02/Performance。
- 初始样本Wizard-PIE-Baseline.csv：19帧/36.115秒，最大16850ms，>100ms 9次、>500ms 7次，分析器退出1。未攻击。当前日志D3D12 3D/Compute queue出现5～16秒timeout，停止PIE后仍有发生，尚不以时间关联认定ArrowTrail根因。
- 临时排除后台节流：经MCP设置bThrottleCPUWhenNotForeground=false(save=false)、Slate.bAllowThrottling=0。Unthrottled样本67帧/21.702秒，最大14262ms、>500ms 4次，仍失败。此为采样条件控制，不记为修复或持久设置。
- 保持其他条件，仅临时fx.NiagaraAllowGPUParticles=0（原值1）后再启动：13帧/67.624秒，最大30734ms、>500ms 7次，仍失败；已恢复1。禁用Niagara GPU模拟不足以消除故障，不能贸然改特效资产。
- 已经MCP采集并停止Saved/T02/Performance/PIE-Stall.utrace，使用本机UnrealInsights -NoUI离线导出，继续定位CPU/Render/RHI等待。当前尚未修复，不声明性能验收通过。

- 后续证据：冷重启首PIE最大21414ms；同窗口等待链GT/RT/RHI → Worker AllocatePooledTextures(17.3819秒) → CreateCommittedResource(该窗口累计17.3800秒)。关闭Aftermath的冷启动对照仍最大24913ms，资源创建累计40.7657秒；不能据此认定Aftermath根因。尚未完成显存分配策略对照。
- 用户随后明确反馈“重启之后就好了”，并要求继续副本粒子改色，按新指示暂停性能排查。新一轮实时核对：ParallelSetup=1、Niagara GPU粒子=1、CSV压缩=-1；未落盘RTUAVTextureMaxAllocationSize试验。当前DefaultEngine.ini中NvAfterMath=0且运行值0，本次不改此设置，不把重启恢复描述为代码或参数修复已证实。性能根因保持未证实；后续若复发可使用现有CSV/trace工具复现。

### 2026-10-28 Free_Magic 普攻副本蓝紫色（用户确认）

- 用户认为ArrowTrail过小，已自行复制`/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_BasicProjectile1_Base`，要求改成蓝紫色。开始时BP_WizardBasicProjectile.Trail.Asset仍指向原版NS_Free_Magic_Projectile1_Base；本步修改副本并接入该引用，保留用户选择的大小、运动、寿命与原包资源。
- 所有Niagara/蓝图/配置操作仍使用UE-MCP；不继续修改渲染参数或性能实验。先核对真实颜色驱动，再做局部颜色调整、编译保存和视觉验证。

- 实施：保留原Alpha/HDR峰值，只改副本User.Color_REF=(35,8,100,1)、User.Color_Sparks1=(25,10,100,1)、User.Color_Trail=(22.5,4,50,2)。三个Emitter的SpawnScript参数读回分别消费对应User颜色；未使用Smoke/Spiral颜色、非颜色参数、共享材质与原版系统未改。MCP局部字节差异核对仅RGB变化，组件缩放仍(.5,.5,.5)。
- 编译/保存：副本8个脚本UpToDate、0错误0告警，Niagara验证3/3，BP编译成功0告警；指定系统与BP保存0失败。首次改Trail.Asset引用后，游戏内截图仍橙色；读回发现组件OverrideParameters保留旧橙色缓存，未把仅资源读回当作视觉通过。
- 用户随后明确反馈“粒子特效我已经替换了，现在已经是蓝紫色了”，以用户当前替换结果作为视觉确认。已中断后续组件缓存同步/覆盖操作，保留用户最新配置；不继续改色、重绑或操作PIE。
- Standards：资产修改全程UE-MCP，表现数据保持资产配置，无C++/GAS/动画/渲染设置改动。Spec：本次副本蓝紫色效果由用户确认；伤害、速度、生命周期与大小未由本步修改。当前地图和原ArrowTrail有用户未保存改动，未为调色关闭编辑器或运行Cook/独立包，不能将此前包的验证延伸到此副本；成品新引用验证保留在T02后续验收中。本票仍in-progress，未提交/推送。

### 2026-10-28 NS_Free_Magic_Hit3 蓝紫色

- 用户要求直接调整`/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit3`，配色沿用已确认投射物。本步仅改该资产，不创建副本、不自动替换GC或蓝图引用。
- UE-MCP读回确认两个active emitter Sparks1/Flash分别消费User.Color_Sparks1/User.Color_Flash；设置为(25,10,100,1)/(17.5,4,50,1)。各自HDR峰值100/50及Alpha1保持原值。仅14个RGB字节变化，其余store字段、缩放、运动、未用Smoke/Spiral颜色和共享材质不变。
- 编译与保存：6个Niagara脚本UpToDate、0错误0告警，validate2/2；仅此系统savedCount1、failed0、stillDirty=false。没有修改代码、组件缓存、GC、蓝图或渲染配置。
- 尝试新瞬态PIE组件视觉检查：PIE请求启动并读到玩家；随后用户打开“选取父类”模态窗口，MCP拒绝时间缩放、生成组件和截图，三项均未执行。没有关闭或回答用户对话框，也没有假称完成像素预览。未对现有组件的旧颜色OverrideParameters做覆盖；用户替换时若仍沿用旧组件缓存，需要重新选择资产或重置对应颜色覆盖。
- Standards：全程MCP、仅指定资产的颜色数据、原强度/Alpha与非颜色数据保持，编译/校验/保存通过。Spec：蓝紫颜色参数及消费绑定读回通过；实际像素预览本次因用户编辑器对话框未执行，未重跑Cook/独立包（未新增引用或修改业务）。用户地图、ArrowTrail和Hit1的未保存改动保留，T02仍in-progress，未提交/推送。

### 2026-10-28～29 普攻 AbilityTask / 蒙太奇出手事件迁移（本步完成）

- 用户认可并授权由subagent实现：GA管理蒙太奇与异步任务，AnimBP继续Blueprint，资源仍蓝图配置。替换原“立即发弹丸→角色OnBasicAttack播蒙太奇→立即EndAbility”链路。
- 目标链路：Commit → WaitGameplayEvent先监听 → PlayMontageAndWait → Montage Notify发Event.Wizard.BasicAttack.Fire → 生成一发弹丸 → 完成/打断/取消统一结束清理。GA配置AttackMontage，原蒙太奇速率/Slot/分层与用户选定的视觉资源保留。
- 输入语义：普通松开只清持续连射意图，已经开始的一发可完成，保证短单击仍发一次；输入Canceled、UI暂停、死亡、Avatar解除与EndPlay强制取消当前GA并阻止晚到通知。冷却仍蓝图GE唯一数值来源；续发同时等冷却与本次GA结束，延至下一tick尝试，避免重入或漏掉下一发。
- 本步不改伤害公式、出手偏移配置、粒子色彩/大小或地图布局，不新增四槽技能。工程验证需真实UHT/UBT、异步/取消/续发自动化、MCP资产读回/编译/保存、实际动画Notify运行与Windows Cook/独立包。物理输入与业务反射测试分开记录。

#### 已执行证据

- C++：GA配置AttackMontage，PlayMontageAndWait与WaitGameplayEvent建立本次执行；Native AnimNotify_WizardBasicAttackFire同步分支通知发送Event.Wizard.BasicAttack.Fire。实际Avatar/当前Montage/ASC动画归属核验，一次激活先消费出手标记再Spawn防重入。EndAbility遵守GAS ScopeLock并统一清Task委托；PS双门控下一tick续发，普通Release与强Stop分离，Controller Completed/Canceled分别接入。角色OnBasicAttack接口与旧播放链移除。
- 保存与冷编译：经MCP保存用户已修改的ArrowTrailMagic和FreeMagicHit1（2saved/0failed），0脏包后正常关闭。BuildEditor `Saved/T01/20261028-234109-BuildEditor-a7d7ee` 退出0，UHT生成9处，18项实际编译链接，34.4秒。MCP重开ready45.5秒；未使用Live Coding替代反射构建。
- 资产全部MCP：GA_BasicAttack.AttackMontage=AM_WizardBasicAttack；唯一BasicAttackFire通知在基准0.4秒、native类/Script/Wizard.AnimNotify_WizardBasicAttackFire，BranchingPoint=true且Marker数1。原RateScale3.3、Blend.03/.04、DefaultSlot、Attack01动画段与ABP绑定不变，实际出手约.121秒。BP_WizardCharacter仅删除旧事件/播放两个节点，保留其余用户内容；相关BP编译0告警、保存0失败。
- UE-MCP `run_automation_tests(filter=Wizard.,maxTests=10)`：3 passed/0failed/0abandoned，各0错误0告警。
- 第二轮PIE Smoke通过：首发同步0弹丸/0伤害且Release后GA/Montage仍active；真实Notify后SpawnDelta1，重复事件2次不增发，结束Ability/Task/Montage归零。GE伤25、实际Hit3 GC峰值1；抑制GC仍伤25/峰值0。2.101秒连射6发（最低4）；强cancel/menu/death出手前均SpawnDelta0/Task0；Avatar替换/幂等授予通过；运行Montage放慢10倍后CooldownEndedWhileActive=1，3.174秒仍续发2次，最终ActiveTrails0，WIZARD_COMBAT_SMOKE_PASS。此binary不含后补的独立Montage_Stop测试阶段，最终Package会重编并补验。
- 首轮PIE Smoke在GC自然归零15秒墙钟限制处失败，当时后台节流实际3fps且有启动渲染等待；之后同池化Hit3组件IsActive=false。只临时关闭后台CPU/Slate节流、保持原断言后第二轮通过，未改特效生命周期或放宽断言。不把工具调用的墙钟与模拟时间混淆。
- 游戏窗口截图`Saved/T02/AbilityMontage/window-tap-preview.png`可见原蓝紫投射物与攻击姿势。尝试CUA窗口短按期间检测到用户输入，已停止自动窗口操作，该过程不作为无人干扰的单击/长按验收证据；没有通过UI修改资产。
- 关闭编辑器前的操作失误：尚未先结束PIE就调用MCP save_dirty(includeMaps=true)，工具把UEDPIE临时世界当作地图保存，触发SavePackage断言并退出；断点在UE_MCP_Bridge EditorHandlers.cpp:3351，非GA游戏链崩溃。事后核对正式地图仍为用户23:13:05版本，无UEDPIE文件落盘；GA/AM/角色BP在此前已保存。后续必须先明确stop PIE并确认isPlaying=false，再保存具体正式包；本步没有修改桥接源码。当前用已落盘资产继续统一Package验证，完成后重开编辑器。

#### 最终交付与双轴检查

- Package：`Saved/T01/20261028-235759-Package-c33490/result.json`退出0，19项实际构建（含新增Notify的Game目标与最终Montage_Stop测试的Editor/Game目标），Cook 0 error(s)/0 warning(s)，Stage/Package/Archive成功。UFS清单包含AM_WizardBasicAttack、蓝紫NS_Free_Magic_BasicProjectile1_Base与NS_Free_Magic_Hit3。成品`Saved/Packages/20261028-235759-Package-c33490/Windows/`。
- 独立包业务Smoke：`Saved/T01/20261029-000028-Smoke-d34f3b/result.json`退出0、bridgeLoaded=false。日志真实Notify SpawnDelta1/重复2不增发；GE伤25/GC1、抑制GC伤25/GC0；2.108秒连射6发；cancel/menu/death均SpawnDelta0/Task0；新增真实Montage_Stop中断也SpawnDelta0/Task0；慢Montage冷却先结束、3.156秒仍续发2次；最终ActiveTrails0、Montage/GA/Task无残留，WIZARD_COMBAT_SMOKE_PASS。
- 恢复编辑器：MCP重新启动ready15.1秒，GA.AttackMontage与ProjectileClass读回正确、0脏内容/0脏地图；已打开GA和原Montage供用户检查。正式地图文件555056字节，时间仍为2026-10-28 23:13:05，未覆盖布局。临时PIE时间缩放恢复1，后台CPU与Slate节流恢复原值；本轮未改渲染配置。
- Standards：C++能力负责任务/事件/生成与取消，原生Notify仅桥接时机，动画蓝图与资源配置保持Blueprint；Task/委托/续发Timer有生命周期清理，事件校验及重复消费防重入，Ability ScopeLock契约保留。源码与本轮工程说明diff-check通过（既有CRLF提示除外），UHT/UBT、MCP资产编译保存、3项自动化、Cook与独立包通过。MCP批量保存PIE事故与安全后续流程已如实记录，没有伪称工具链完全无问题。
- Spec：R02/R07/R08本步AbilityTask+真实出手Notify、普通松开/强取消、两种冷却先后次序通过；R03/R04业务暂停取消通过。用户已认可的普通松开语义同步到规格。完整物理输入、CommonUI返回焦点和整票其他待办未由反射Smoke替代；CUA尝试受用户同时操作干扰，不计独立输入验收。T02仍in-progress，未提交/推送。
- 学习资料：`.local-learning/lessons/2026-10-28-ability-montage.md`说明调用链、Task激活、出手事件、释放/取消与双门控续发。用户掌握度未测试，Execution捕获待补讲状态保留。

### UIRoot 实施（2026-10-29）

- 用户确认固定三层：HUD 常驻展示、Menu 常规菜单、Modal 模态界面；后续界面复用这三层，不新增层。
- 本次将 Controller 分别 AddToViewport 的方式改为唯一 UIRoot 入口，保留 GAS 属性事件绑定；Menu、Modal 使用 CommonUI 容器，暂停按两层的总体占用状态协调。
- 验收范围：根层和现有 HUD/暂停菜单接入、嵌套关闭恢复、世界输入隔离与销毁清理。验证结果待实现后填写，不提前判定 R03/R04 完成。

#### UIRoot 验证结果（2026-10-29）

- 实现：`UWizardUIRoot` 绑定 WBP_UIRoot 的 HUDLayer、MenuLayer、ModalLayer；Controller 仅把根加入 viewport，生命 HUD 加入 HUDLayer，暂停菜单压入 ModalLayer。Menu/Modal 使用 CommonActivatableWidgetStack；Modal 打开时禁止 PushMenu 并禁用 Menu 子树。两层总体状态驱动暂停，最后关闭恢复此前状态；原有业务测试入口与真实根层状态合并，避免直接业务调用提前解除根层暂停。
- 资产：通过 UE-MCP widget.create/build_tree 创建 `/Game/Wizard/UI/WBP_UIRoot`，三层全屏叠放，蓝图编译 upToDate，保存成功；未修改地图。
- UHT/UBT：`pwsh -NoProfile -File Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor` 退出 0，实际编译与链接，证据 `Saved/T01/20261029-125954-BuildEditor-a93502/`；非干净重编译。
- 自动化：同入口 `-Action Test` 退出 0，3 passed、0 failed、0 notRun，证据 `Saved/T01/20261029-130012-Test-1eca2f/`。
- PIE：通过 MCP 启动现有开发地图，再通过 KismetSystemLibrary.ExecuteConsoleCommand 在 PIE world 执行 `Wizard.CombatSmokeTest`。日志 `Saved/UIRoot/20261029-pie.log` 出现 `WIZARD_UIROOT_STACK_PASS`：真实 Menu/Modal 栈嵌套、阻止模态期间打开 Menu、关闭 Modal 保持暂停、关闭最后 Menu 恢复、打开前已暂停则关闭后仍暂停，全部通过。
- 运行树读回：WBP_UIRoot_C_0 在 viewport 且可见；WBP_CombatHUD_C_0 可见且不再直接在 viewport。原战斗 Smoke 改为检查所属 Controller、根 viewport 与 HUD 父节点。
- 完整战斗 Smoke 未通过：phase=2 连射观察窗口 2.334 秒内 3 次命中，最低要求 4 次；不得把本轮称为完整战斗回归通过。UIRoot 检查位于该失败之前，单独通过。
- Standards：UE5.4 本机容器 API、真实 UHT/UBT、资产 MCP 修改、固定三层与生命周期清理检查通过；物理输入路由尚未验收。
- Spec：R03 的栈式暂停/关闭恢复通过上述接口运行检查；R04 的物理 Esc、焦点恢复、鼠标点击隔离、按住/松开仍未验收。未运行本次独立包/Cook，不勾选 T02 整体验收。
- 学习路径：Controller.BeginPlay → UIRoot.SetHUD；OpenMenu → PushModal；栈顶变化 → RefreshScreenState → Controller.RefreshMenuState。蓝图仅配置布局；后续背包/技能/商店复用 Menu，暂停/确认复用 Modal。

### 玩家状态 HUD 重做（2026-10-29）

- 用户明确取代旧 HUD：生命/魔力进度条显示比例，对应右侧文本为“当前值/最大值”；状态模块位于左下角；移除目标生命与按键文案。
- WBP_CombatHUD 直接继承 CommonUserWidget，仅聚合 WBP_PlayerStatus；不设置属性绑定或业务图表。C++ 状态模块负责 GAS 四属性事件、初始刷新、解绑和重新构建。UIRoot 继续固定 HUD/Menu/Modal 三层。
- 通过 UE-MCP 创建 WBP_PlayerStatus 并重做 WBP_CombatHUD；蓝图编译 upToDate，左下锚点(0,1)、对齐(0,1)、边距28，生命红条、魔力蓝条，文本位于右侧。最终构建/运行证据待填写。

#### 玩家状态 HUD 验证结果（2026-10-29）

- Standards：通过。C++ 监听 Health/MaxHealth/Mana/MaxMana 事件并在析构解绑；NativeConstruct 从所属玩家取 PlayerState。聚合蓝图直接继承 CommonUserWidget，无业务图表；Root.SetHUD 接受通用 UUserWidget。资产修改全部通过 UE-MCP；地图未修改。移除旧目标选择与 HUD 接口，历史装配入口不再生成旧 HUD。
- 构建：`pwsh -NoProfile -File Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor`，`Saved/T01/20261029-132313-BuildEditor-02b74a` 退出0，实际 UHT/UBT 编译。截图发现动态 OverlaySlot 默认 Left/Top 导致全屏 Canvas 锚点错误，改为 Fill/Fill；最终 `20261029-132548-BuildEditor-2ce394` 退出0。过程中一次过早启动构建被编辑器存活保护拒绝，一次本地变量 Slot 遮蔽父成员被编译器拒绝；均已修复。
- 自动化：`-Action Test`，`Saved/T01/20261029-132338-Test-b86b1b` 退出0，4 passed/0 failed。新增 Wizard.UI.PlayerStatusAttributesAndLifecycle 检查四属性事件、实际进度条百分比/文本、切换来源后旧事件无效、销毁解绑、重建、未绑定0/0。该测试运行在最终 OverlaySlot 布局修正前；布局修正通过后续 PIE 截图直接验证。
- Spec/用户五项要求：本次范围通过。UE-MCP 在既有开发地图启动 PIE，运行实例 PlayerStatus 归属于 WizardPlayerController_0，真实 HealthBar/ManaBar Percent=1，HealthText/ManaText=100/100。两蓝图编译零错误零警告。截图 `Saved/HUD/20261029-player-status-final.png`（1352x1203）确认左下角两条、右侧文本、红蓝颜色、无目标血量与按键提示。初次截图位置及颜色不正确，已修正并读回颜色与重新截图。
- C++ 保留 UWizardHealthWidget 类名兼容资产，但职责已改为玩家生命/魔力状态模块；资产入口为 `/Game/Wizard/UI/WBP_PlayerStatus`。父聚合容器为 `/Game/Wizard/UI/WBP_CombatHUD`，后续 HUD 模块直接在其 Canvas 中聚合。
- 未执行本次 Cook/独立包和全战斗 Smoke；本轮未改游戏输入，物理输入/焦点验收继续保留未完成。T02 整体仍 in-progress。原始本地截图/日志由 Saved 忽略，复跑命令及结果摘要在本票据版本控制中。

### 玩家状态类命名（2026-10-29）

- 用户要求：UWizardHealthWidget 重命名为 UWizardPlayerStatusWidget，文件同步为 WizardPlayerStatusWidget.h/.cpp；测试和 Smoke 引用全部更新。
- 通过 UE-MCP 在 DefaultEngine.ini 添加旧类到新类的 CoreRedirect，保证已有资源加载；重启后 MCP 确认 WBP_PlayerStatus 父类为 /Script/Wizard.WizardPlayerStatusWidget。玩家状态/聚合 HUD 蓝图编译均零错误零警告，并强制保存资源。
- Standards：实际 UHT/UBT BuildEditor 退出0（Saved/T01/20261029-133545-BuildEditor-0795ad），Wizard.UI 定向自动化通过（Saved/T01/20261029-133602-Test-bd98a4）；Source 内无旧类引用，只有迁移重定向及历史记录保留旧名。
- Spec：仅命名与序列化引用迁移，不更改 HUD 行为；蓝图父类和加载验证通过，本轮未重复完整玩法/独立包验收。替代上一节为兼容而保留旧类名的临时决定。

### 本轮版本快照范围（2026-10-29）

- CommonUI UI Action 已配置 Enhanced Input：DefaultGame 开启 bEnableEnhancedInputSupport；IA_UIBack 对应 Escape，IA_UIConfirm 对应 Enter/LeftMouseButton；两动作允许暂停触发；BP_UIInputData 引用返回/确认动作，WBP_PauseMenu 配置 IMC_UI、优先级100，由可激活界面生命周期管理映射。仅资产配置完成；此前用户停止电脑操作，实际 Esc/焦点/点击隔离未验收，不能以自动化替代。
- 用户授权提交并推送当前版本。包含 UIRoot、玩家状态 HUD、UI 增强输入配置、命名迁移及测试/证据摘要；不包含既有未跟踪 Config/DefaultDefaultEngine.ini，不包含 Saved 原始输出。本次版本快照不改变 T02 未完成验收状态。
