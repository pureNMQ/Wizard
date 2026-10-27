# 工作流、架构与质量标准草案

状态：供开工前审阅。本文件中的技术提案尚未自动成为已确认的游戏规则；正式 AGENTS.md 与 docs/agents/ 配置尚未写入。

## 1. 已确认的工作方式

- 使用 Git，用户在正式开工前创建 GitHub 远程仓库。
- 任务和规格保存在本地 Markdown 中，随项目版本控制；GitHub 远程不等于自动切换到 GitHub Issues。
- 规格的唯一正文为 [Demo 规格](../../.scratch/wizard-demo/spec.md)。已确认规则以该文档为准，变更后同步验收条目。
- 每次交付一个可运行、可检查的小功能，附修改理由、验证证据和未解决问题。
- 学习文件放在 `/.local-learning/` 并由 Git 忽略；工程测试与项目说明纳入版本控制。
- 最终截止日期是 2026-11-03。范围调整由用户决定，不静默删除基础要求。

## 2. 规范的三个层级

| 层级 | 判断依据 | 例子 |
| --- | --- | --- |
| 引擎/API 契约 | UE5.4 官方文档及本机 5.4 源码 | UObject 生命周期、Ability 的结束与取消 |
| 框架推荐实践 | 官方说明及适用条件 | GameplayEffect 管理战斗数值变化、CommonUI 输入路由 |
| 项目约定 | 本项目真实需求和明确取舍 | ASC 放在哪里、数据结构、中文注释、事务和存档策略 |

审查说明必须标明依据属于哪一层。ASC 位于 PlayerState 或 Character 都是合法方案，不能把某个示例工程的选择称为通用强制规范。

Epic C++ 规范是基线；中文注释和文档是用户明确指定的例外。网页可能默认显示较新引擎版本，具体 API、弃用提示与可用功能以 UE5.4 为准。

## 3. 源码检查项

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

## 4. GAS 检查项

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

## 5. CommonUI、输入与暂停检查项

- 配置 UCommonGameViewportClient 或其正确派生类；背包、技能与商店使用可激活界面及清晰的层级管理。
- CommonUI 处理返回、焦点和输入路由；游戏操作通过 Enhanced Input 的 Action/Mapping Context 配置。
- 游戏输入与 UI 输入的连接方式先在 UE5.4 做最小验证；不直接把新版示例当作该版本已验证方案。
- 暂停状态集中管理，嵌套界面关闭一个后仍保持需要的暂停；最后一个界面关闭时恢复此前状态。
- 暂停时 UI 的点击、拖拽、返回和交易可用；世界输入不会穿透；界面关闭后移动与连射正确恢复。
- 拖拽携带稳定物品身份和来源信息；放下时重新校验，成功后统一提交状态、广播刷新并请求保存。
- 拖拽取消、无效目标、堆叠溢出、背包已满的换装和关闭界面期间的拖拽都保持物品守恒。
- 反复开关界面不会重复绑定事件；数值与技能冷却显示读取实际业务/GAS 状态。

## 6. 数据、物品与存档检查项

- 配置定义和运行实例分离。技术提案：技能/物品定义采用 Primary Data Asset；需要表格批量配置的目录、掉落和波次采用 Data Table 或组合资产。
- 定义 ID、运行时物品 ID、背包位置和装备位置的含义分开；任何一件运行时物品只有一个有效归属。
- 堆叠、拆堆、拾取、丢弃、换装、买卖使用可验证的完整事务：先校验整体结果，再一次提交；失败时金币与物品状态一致。
- 检查负数、零数量、越界、错误槽位、不存在的定义、非法价格和重复物品身份。
- 出售素材复用物品定义、图标、堆叠和商店价格机制；新增同类素材无需写新的出售分支。
- 地面图标引用实际物品定义；图标和自动拾取在 Windows 打包中可用。
- Primary Asset 扫描、软引用加载与 Cook 规则一并验证；编辑器中可用不等于打包后可用。
- 存档使用可序列化的数据快照、稳定定义 ID 与必要版本信息；恢复运行时对象和装备效果时统一重建。
- 技术提案：首版一个本地存档槽，使用 USaveGame；自动存档按版本序列化排队，异步写入期间快照保持不变，旧快照不能覆盖新进度。
- 保存失败和损坏/不兼容存档有明确诊断并保留恢复依据；界面报告真实完成状态，不把已排队误报为已落盘。

## 7. 待审阅的职责方案

这是工程提案，不是 GAS 的唯一规范；名称和最终类划分在首个实现任务中落定。

| 责任 | 提案 | 理由/验证重点 |
| --- | --- | --- |
| 玩家持续状态 | PlayerState 持有 ASC、AttributeSet 和玩家物品状态组件 | 角色重生后仍可保留状态；Character 为 Avatar，验证重新绑定 |
| 角色与控制器 | Character 负责移动/表现，Controller 接收输入并请求业务 | 避免把背包、交易和战斗规则集中到控制器 |
| 物品域 | 小接口统一协调背包、装备位置与金币事务；装备适配层管理 GE | 守恒和失败回滚可通过接口测试 |
| 商店 | 配置目录与交互入口，调用玩家物品事务接口 | 使用同一套物品定义与价格，不另建库存副本 |
| 敌人与战斗 | 敌人 Character 持有自身 ASC；C++ AI 行为与挑战控制器 | 两类行为、波次状态和一次性死亡/掉落处理 |
| UI | C++ CommonUI 基类与事件绑定，Widget Blueprint 配置布局/样式 | 可以验证输入路由，同时保留数据与资源配置 |
| 保存 | 有明确生命周期的保存服务与 USaveGame 快照 | 记录顺序、恢复校验和退出前保存完成 |

单个游戏运行时模块按功能划分目录起步；只有实际依赖或编译隔离收益明确时才拆更多模块。

## 8. 每个任务的完成条件

1. 写出对应规格编号、输入/输出、不变量和失败情形。
2. 关键规则先通过公开接口建立有意义的失败测试，再实现最小可用行为；资源和界面表现用实际运行补充验证。
3. 运行受影响的构建、测试和必要编辑器检查；记录命令、结果和证据路径。
4. 分开检查“符合规范”与“符合规格”；修复阻塞项后再提交。
5. 提交摘要包含修改理由、验证结果、已知限制；未运行检查明确标注。
6. 更新用户需要理解的调用路径与学习任务，写入本地忽略目录。

正式验收至少覆盖：完整循环、全部六个技能与四槽搭配、背包/拖拽事务、装备数值、商店、出售素材、掉落图标、自动拾取、死亡、存档往返、暂停输入及独立游戏包。

## 9. 正式工作流文件的内容草案

用户已接受使用 AGENTS.md 与本地任务文档。以下内容待整体审阅后写入对应位置；当前仅本草案生效于规划讨论，不表示已经完成工具配置。

### AGENTS.md 提案

```markdown
# Wizard 项目指引

实现或审查功能前，阅读 `.scratch/wizard-demo/spec.md` 的相关编号；处理源码与框架使用时阅读 `docs/CODING_STANDARDS.md`。
规格中的已确认规则与技术提案分开处理；改变基础行为前与用户确认。
英文标识符，中文注释和项目文档。学习笔记、练习、个人测验及答案写入 `.local-learning/`，工程测试纳入版本控制。
每个任务交付可运行的小功能，记录验证结果并检查规范与规格两条轴线。

## Agent skills

### Issue tracker

规格与任务位于 `.scratch/<feature>/`，使用本地 Markdown。具体约定见 `docs/agents/issue-tracker.md`。

### Domain docs

采用单一术语上下文。涉及术语或设计决策时读取 `GLOSSARY.md` 和相关 `docs/adr/`；消费规则见 `docs/agents/domain.md`。
```

`docs/CODING_STANDARDS.md` 将承接本草案第 2～8 节，保持规范只有一份正文。

### docs/agents/issue-tracker.md 提案

```markdown
# 本地任务追踪

规格位于 `.scratch/<feature>/spec.md`；实施计划位于同目录 `plan.md`。
每个任务独立保存到 `issues/<NN>-<slug>.md`，从 01 按依赖顺序编号。
任务包含 Status、Blocked by、规格编号、验收条件与验证记录。
Status 使用 draft、ready、in-progress、blocked、done；这些是本项目约定，未启用 triage 技能。
完成条件为规范审查、规格验收和相应验证均有证据；失败或未执行的检查如实记录。
“发布到任务追踪”在本项目中表示写入上述本地文件，不自动创建 GitHub Issue。
```

### docs/agents/domain.md 提案

```markdown
# 领域文档

涉及领域术语时先读根目录 GLOSSARY.md；涉及重要取舍时读取相关 docs/adr/。
文件尚不存在时继续工作，有真实术语或决策需要记录时再创建。
术语表只描述游戏概念；实现取舍放在规格、规范或 ADR 中。
ADR 仅用于难以逆转、没有背景会令人困惑且存在真实取舍的决定。
发现冲突时说明冲突及影响，更新经过确认的规则，避免并存两份不同结论。
```

## 10. 首日环境与版本管理任务

- 用户提供 GitHub 仓库地址和可见性；在此之前不编造远程地址或推送目标。
- 初始化本地 Git；检查 Git LFS 可用性，为实际纳入仓库的二进制资产设置规则，检查暂存清单。
- 补充 UE 生成目录、IDE 缓存、打包输出和本地配置的忽略规则，保留既有学习目录规则。
- 梳理第三方素材的来源、许可与交付方式；仓库公开方式以实际授权和用户选择为依据。
- 验证本机 UE5.4、编译器、SDK、UHT/UBT 和打包工具；固定可重复的构建、测试、打包入口。
- UE_MCP_Bridge 等开发工具插件的打包行为通过实际构建核实，运行包不依赖用户的编辑器连接。

## 11. 参考依据

- [Epic C++ 编码规范](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine)：命名、组织及格式基线；语言例外见本文件。
- [UE5.4 ASC 与属性](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-system-component-and-gameplay-attributes-in-unreal-engine?application_version=5.4)：所有权、接口和共享 ASC。
- [UE5.4 Gameplay Ability](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-abilities-in-unreal-engine?application_version=5.4)：激活、提交、结束、取消和实例化策略。
- 本机 `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Public/Abilities/GameplayAbility.h`：CommitAbility、EndAbility 等接口以实际 5.4 头文件为准。
- [UE5.4 CommonUI 输入](https://dev.epicgames.com/documentation/en-us/unreal-engine/commonui-input-technical-guide-for-unreal-engine?application_version=5.4)：Viewport、激活树及输入路由。
- [UE5.4 资产管理](https://dev.epicgames.com/documentation/en-us/unreal-engine/asset-management-in-unreal-engine?application_version=5.4)：Primary Asset、加载及 Cook 管理。
- [官方存档说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/saving-and-loading-your-game-in-unreal-engine)：USaveGame 与同步/异步保存；具体签名核对 UE5.4。
