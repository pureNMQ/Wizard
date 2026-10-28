# 编码与框架规范

生效日期：2026-10-28。本文是项目实现和规范审查的正式依据；由原工作流草案迁移。适用于当前 UE5.4 单机 Demo。

玩法及验收以 [规格](../.scratch/wizard-demo/spec.md) 为准，本文定义实现质量约束，不新增玩法。具体职责归属、资产格式、存档结构等未定方案集中于 [技术提案](planning/implementation-proposals.md)，不随本规范发布自动成为既定架构。

修改源码或框架接入时，先读第 1、2 节，再读受影响的 GAS、输入/UI 或数据/物品/存档章节。审查时区分下列三种依据，记录实际采用的 UE5.4 API、验证结果与适用条件。

## 1. 规范的三个层级

| 层级 | 判断依据 | 例子 |
| --- | --- | --- |
| 引擎/API 契约 | UE5.4 官方文档及本机 5.4 源码 | UObject 生命周期、Ability 的结束与取消 |
| 框架推荐实践 | 官方说明及适用条件 | GameplayEffect 管理战斗数值变化、CommonUI 输入路由 |
| 项目约定 | 本项目真实需求和明确取舍 | ASC 放在哪里、数据结构、中文注释、事务和存档策略 |

审查说明必须标明依据属于哪一层。ASC 位于 PlayerState 或 Character 都是合法方案，不能把某个示例工程的选择称为通用强制规范。

Epic C++ 规范是基线；中文注释和文档是用户明确指定的例外。网页可能默认显示较新引擎版本，具体 API、弃用提示与可用功能以 UE5.4 为准。

## 2. 源码检查项

- 蓝图、资产及其配置的修改必须通过 UE-MCP 执行（2026-10-28 用户明确要求）。连接失败时先修复连接，禁止改用外部 Python 脚本、命令行资产装配或界面操作绕过 MCP。历史装配脚本仅供来源参考，不再作为这些修改的执行入口；C++ 编辑、构建和只读日志诊断不受此限制。

- 按 UE 类型前缀、PascalCase、布尔 b 前缀、明确动词函数名和独立花括号组织代码；使用 Tab 缩进并保持当前模块一致。
- 标识符、资产名使用英文；注释解释约束、原因和边界，使用中文。公开业务接口说明输入、失败结果和副作用。
- 使用 const 表达只读约束；序列化数量、货币和版本使用明确宽度的整数类型；价格和数量运算检查范围。
- 头文件只包含必要依赖，generated.h 保持正确位置；cpp 先包含对应头文件；通过真实 UHT/UBT 构建验证。
- 模块依赖按公开接口是否暴露该类型区分 Public/Private；运行时模块保持对 Editor 模块的隔离。
- UObject 引用根据持有关系采用反射可见的强引用或弱引用；说明生命周期，避免把普通 C++ 智能指针用于管理 UObject。
- 委托、定时器、异步请求和 AbilityTask 有明确解除/取消路径；界面关闭、Actor 销毁和角色重生后不会收到失效回调。
- 业务失败返回可区分的结果及必要日志；关键不变量使用适当断言。日志不通过每帧重复错误掩盖问题。
- Actor、Controller、Widget 各自职责明确；UI 通过业务接口修改状态，属性和背包展示使用事件刷新。
- Tick 仅用于实际需要的持续更新；数量、价格、概率、冷却、资源引用来自可校验配置。
- 新增自有代码保留真实来源信息；保留第三方原有版权与许可，禁止把模板版权文字冒充新代码作者信息。

- 动画蓝图保持使用 Blueprint；C++ 提供移动与战斗状态，动画状态机、混合和动画接线保留在动画蓝图（2026-10-28 用户补充）。

## 3. GAS 检查项

- ASC 的 Owner、Avatar 和 IAbilitySystemInterface 返回值一致；占有、初始化、重生时序经过验证，能力授予幂等。
- 通过 ASC 授予和尝试激活 Ability；能力实现处理 CanActivate、Commit 返回值、正常 End、Cancel 与失败清理。
- 能力实例化策略与成员状态、委托、异步任务的使用相符；复用的实例在结束时清理每次执行状态。
- 消耗、冷却和激活限制使用 GAS 机制；生命、法力与战斗属性只有一份权威状态。
- 伤害、治疗、减速、护盾和装备加成使用合适的 GameplayEffect/Spec；运行时数据放在 Spec 或实例状态中，配置资产/CDO 保持只读。
- GameplayTag 用于技能和状态语义；需要自动解除的状态有清晰的效果或生命周期所有者，避免散落的重复布尔状态。
- 按需要使用 AbilityTask、动画通知或 GameplayEvent 协调异步行为；结束、死亡或中断后清理任务与输入状态。
- 装备持有效果的 Handle，卸下时准确移除；读档和重生只建立一次有效装备加成。
- 生命等属性处理上下限；减小最大生命按规格夹取当前值；死亡响应保证一次处理。
- 普通攻击和技能命中携带合理的来源/目标上下文；弹丸与范围伤害通过业务规则处理有效目标及重复命中。
- GameplayCue 承担所采用的 GAS 效果表现，核心伤害结算独立于表现是否播放成功。
- 网络执行和复制策略服务当前单机范围；保留框架必需的权限语义，不宣称已经验证多人或客户端预测。

必须实际验证：资源不足、冷却中、无效目标、施法取消、死亡时技能仍在执行、重复初始化和连续输入。

## 4. CommonUI、输入与暂停检查项

- 配置 UCommonGameViewportClient 或其正确派生类；背包、技能与商店使用可激活界面及清晰的层级管理。
- CommonUI 处理返回、焦点和输入路由；游戏操作通过 Enhanced Input 的 Action/Mapping Context 配置。
- 增强输入是基础验收要求：C++ 使用 UEnhancedInputComponent 绑定资产，并由 UEnhancedInputLocalPlayerSubsystem 管理上下文；物理键位通过 Mapping Context 配置。
- 四槽输入解析当前技能搭配并请求 GAS 激活；输入层保持对具体技能类的解耦。检查 Started、Triggered、Completed、Canceled 及角色重建时的绑定生命周期。
- 游戏输入与 UI 输入的连接方式先在 UE5.4 做最小验证；不直接把新版示例当作该版本已验证方案。
- 暂停状态集中管理，嵌套界面关闭一个后仍保持需要的暂停；最后一个界面关闭时恢复此前状态。
- 暂停时 UI 的点击、拖拽、返回和交易可用；世界输入不会穿透；界面关闭后移动与连射正确恢复。
- 拖拽携带稳定物品身份和来源信息；放下时重新校验，成功后统一提交状态、广播刷新并请求保存。
- 拖拽取消、无效目标、堆叠溢出、背包已满的换装和关闭界面期间的拖拽都保持物品守恒。
- 反复开关界面不会重复绑定事件；数值与技能冷却显示读取实际业务/GAS 状态。

## 5. 数据、物品与存档检查项

- 配置定义和运行实例分离。具体资产格式与目录组织参考技术提案，并在相应任务中通过加载和 Cook 验证后落定。
- 定义 ID、运行时物品 ID、背包位置和装备位置的含义分开；任何一件运行时物品只有一个有效归属。
- 堆叠、拆堆、拾取、丢弃、换装、买卖使用可验证的完整事务：先校验整体结果，再一次提交；失败时金币与物品状态一致。
- 检查负数、零数量、越界、错误槽位、不存在的定义、非法价格和重复物品身份。
- 出售素材复用物品定义、图标、堆叠和商店价格机制；新增同类素材无需写新的出售分支。
- 地面图标引用实际物品定义；图标和自动拾取在 Windows 打包中可用。
- Primary Asset 扫描、软引用加载与 Cook 规则一并验证；编辑器中可用不等于打包后可用。
- 存档使用可序列化的数据快照、稳定定义 ID 与必要版本信息；恢复运行时对象和装备效果时统一重建。
- 保存机制确保异步写入期间快照保持不变，旧快照不能覆盖新进度；存档槽、版本及排队方案在相应任务中落定并验证。
- 保存失败和损坏/不兼容存档有明确诊断并保留恢复依据；界面报告真实完成状态，不把已排队误报为已落盘。

## 6. 验证与交付

- 关键业务不变量与失败路径优先建立能复现实际问题的用例，再实现或修复；测试覆盖公开行为与系统边界，避免仅复述内部实现。
- 构建、自动化、PIE、实际输入、性能与成品包各自提供不同证据；按 [验证流程](agents/validation.md) 选择受影响检查。
- 任务完成采用 [本地任务追踪](agents/issue-tracker.md) 的 done 条件，分别填写 Standards 和 Spec 结果。文档或低影响配置改动按实际影响验证，不机械要求重跑全部游戏检查。
- 发布交付前，按计划的验证矩阵检查完整循环、全部技能、背包/装备/交易、自动拾取、死亡、保存恢复、暂停输入和独立游戏包。

## 7. 参考依据

- [Epic C++ 编码规范](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine)：命名、组织及格式基线；语言例外见本文件。
- [UE5.4 ASC 与属性](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-system-component-and-gameplay-attributes-in-unreal-engine?application_version=5.4)：所有权、接口和共享 ASC。
- [UE5.4 Gameplay Ability](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-abilities-in-unreal-engine?application_version=5.4)：激活、提交、结束、取消和实例化策略。
- 本机 `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Public/Abilities/GameplayAbility.h`：CommitAbility、EndAbility 等接口以实际 5.4 头文件为准。
- [UE5.4 CommonUI 输入](https://dev.epicgames.com/documentation/en-us/unreal-engine/commonui-input-technical-guide-for-unreal-engine?application_version=5.4)：Viewport、激活树及输入路由。
- [UE5.4 资产管理](https://dev.epicgames.com/documentation/en-us/unreal-engine/asset-management-in-unreal-engine?application_version=5.4)：Primary Asset、加载及 Cook 管理。
- [官方存档说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/saving-and-loading-your-game-in-unreal-engine)：USaveGame 与同步/异步保存；具体签名核对 UE5.4。
