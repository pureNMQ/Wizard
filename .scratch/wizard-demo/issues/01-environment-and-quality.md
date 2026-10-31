# T01 环境与质量入口

- Status: done
- Blocked by: 无
- Spec: R29、R30、R31、R32、R33、R34，正文见 [规格](../spec.md)
- Updated: 2026-10-28

## 目标与范围

建立可复跑的工程入口、正式开发约定与最小 Windows 成品包，为后续功能验证提供基础。2026-10-28 已完成本票据范围内的构建、自动化、成品打包与启动检查；不代表 R29～R34 的最终游戏交付已完成。

## 输入、输出与不变量

- 输入：已确认规格、已提交场景基线、本机 UE5.4 与现有工具链。
- 输出：正式规范和任务文档、构建/测试/打包调用方式、最小成品运行证据与环境限制。
- 保留用户调整后的 L_WizardClearing 布局；学习资料、本机生成物和凭据留在各自适当位置。

## 失败与边界

检查缺失工具链、构建失败、开发插件依赖、Cook 与独立包资源缺失。已有 CommonUI viewport 告警记录为 T02 的框架接入项；本票据不以启用插件冒充接入成功。

## 验收条件

- [x] Git、远程、LFS 和生成目录/学习目录忽略规则已建立；历史证据见项目基准与场景基线记录。
- [x] AGENTS.md、正式编码规范、任务/领域/验证/学习约定及 T01～T10 票据落地，原草案改为迁移入口。
- [x] 固定并实际复跑 WizardEditor 的 UHT/UBT 构建入口，说明版本和是否增量构建。
- [x] 建立可复跑的工程自动化测试发现/运行入口，以真实存在的测试验证执行链路；测试数量与未覆盖范围如实记录。
- [x] 建立可复跑的 Build/Stage/Package 入口，验证开发插件的成品依赖边界。
- [x] Windows 最小成品包能独立启动当前场景，生成玩家并退出；不宣称完成尚未实现的游戏循环。
- [x] 将环境、素材来源/许可待办和已知限制链接到后续任务，完成本票据双轴审查。

## 验证记录

- 既有环境与 Cook 检查：[项目基准](../../../docs/planning/baseline-2026-10-27.md)、[场景基线](../../../docs/planning/scene-build-2026-10-27.md)；历史范围以原记录为准。
- 2026-10-28：本轮只修改开发约定与任务文档。检查 21 份文档、78 个本地链接，全部有效；10 张票据字段完整、依赖无循环，R01～R34 均有归属，R01～R28 均有 T10 之前的实现任务承接。34 条 R 编号规则逐行与当前 HEAD 比对相同，README 保持空白，游戏源码/配置/资产未改动。
- 本地检查：引擎自带 Python 执行 `Saved/DocumentationChecks/validate_conventions.py`，退出码 0；结果为同目录 `result.json`。该脚本为本轮文档检查用的本地辅助，未作为游戏测试入口交付。`git diff --check` 通过。
- 上述文档记录属于开发约定落地阶段；工程验证结果见下节。

### 2026-10-28 工程入口与成品验证

统一入口：[Invoke-Wizard.ps1](../../../Tools/Build/Invoke-Wizard.ps1)。操作说明、测试边界与输出位置见 [构建与测试说明](../../../docs/BUILD_AND_TEST.md)。以下命令在项目根目录执行，各动作独立成功；All 组合没有另外再跑一整轮。

| 动作 | 命令 | 结果与证据（均位于 Saved/T01 下） |
| --- | --- | --- |
| 编辑器构建 | `pwsh -NoProfile -File Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor` | 退出 0；`20261028-022949-BuildEditor-6b3a86/build-editor-console.log`。实际编译测试并链接 Wizard 模块，非仅 up to date；本轮没有干净重编译引擎 |
| 自动化 | `pwsh -NoProfile -File Tools/Build/Invoke-Wizard.ps1 -Action Test` | 退出 0；`20261028-023036-Test-0ea555/AutomationReport/index.json`，2 成功、0 失败、0 未运行；脚本核实两个预期测试名称和成功状态 |
| 成品打包 | `pwsh -NoProfile -File Tools/Build/Invoke-Wizard.ps1 -Action Package` | 退出 0；`20261028-023248-Package-a2dac4/package-console.log`，Build/Cook/Stage/Package/Archive 均完成；Cook 摘要为 0 error(s)、0 warning(s) |
| 独立启动 | `pwsh -NoProfile -File Tools/Build/Invoke-Wizard.ps1 -Action Smoke -PackageRoot 'Saved/Packages/20261028-023248-Package-a2dac4'` | 退出 0；`20261028-023944-Smoke-bc8cd0/packaged-smoke.log`。真实玩家移动 256.07 厘米并保持落地，命中 WIZARD_SMOKE_PASS，随后正常退出 |

环境：UE 5.4.4（35576357），VS2022 MSVC 14.38.33145 工具链（安装目录 14.38.33130），Windows SDK 10.0.19041.0，UE 自带 .NET SDK 6.0.302，PowerShell 7.6.5。首次编译遇到的模块头文件路径与 TObjectPtr 模板推导问题已修复；失败日志保留在对应早期运行目录。第三方桥接源码的 UE API 弃用告警未通过修改上游源码消除。

成品位置：`Saved/Packages/20261028-023248-Package-a2dac4/Windows/`，为 Win64 Development 包，普通启动入口为该目录的 `Wizard.exe`。这是首次本机成品验证，不是干净机器的兼容性或最终发布验收。

实际启动验证期间交互编辑器已关闭；成品使用 D3D12/SM6 加载当前场景并生成 BP_TopDownCharacter_C。原生截图 `20261028-023944-Smoke-bc8cd0/packaged-startup.png` 已检查，地面/道具与玩家正常显示。较早两轮 Smoke 同样成功，最后一轮在脚本中同时请求 HighResShot 截图。位移来自导航接口，不代表物理鼠标输入已经验收。

`UE_MCP_Bridge` 的项目引用限定 Editor；成品文件清单中没有桥接文件，实际启动日志没有桥接挂载/模块日志。EngineSettings 仅在编辑器目标中用于测试，运行时检查不包含 Editor 模块。普通游戏启动不执行 Smoke，显式开发检查完成才自动退出；Shipping/Test 构建排除此命令。

地图文件 SHA-256 为 `a19abf9b4b26246e7f7f864ea7661ffcb18cc93e9c7acfecf905d9392e6e77b7`，与已提交场景基线一致。验证结束已重新打开 L_WizardClearing，并恢复此前的两项临时编辑器限速设置。

### 双轴审查与后续归属

- Standards：T01 范围通过。脚本对原生命令退出码、自动化报告及真实启动标记分别校验；失败日志保留，测试覆盖启动契约而非空断言；编辑器依赖与开发检查隔离，未改变玩法和手工场景。
- Spec：T01 的七项验收条件均有结果。已具备后续任务使用的构建/测试/打包入口与最小包；R29 的完整循环、存档重启及 R30～R34 最终交付仍由相关任务/T10 验收。
- 已知 CommonUI viewport 路由错误仍交给 [T02](02-basic-combat.md)；本票据没有关闭该告警，也不宣称 UI 路由通过。
- 素材来源、原始页面、许可与署名待办维护在 [素材清单](../../../docs/ASSET_SOURCES.md)，由 [T10](10-final-review-and-delivery.md) 补齐最终交付。
- 自动化仅有两个启动/场景检查，尚不覆盖战斗、物品事务、输入、UI、存档或性能；当前成品仍为模板控制和表现。

## 未决实现与学习

本机入口已通过实际运行落定。构建、Cook、自动化和成品检查的区别与重跑步骤见构建与测试说明；教学解释不计为用户已独立掌握。新增业务功能时逐步扩展真实自动化覆盖。

## 2026-10-31 按测试规范删除六类低价值检查

- 用户在静态审查后要求直接删除六类检查，不重写或增加替代测试。本次删除旧SceneStructure用例、固定能力总数7的断言、只清空局部令牌的假取消检查、技能Smoke及其脚本入口、普攻Smoke中依赖Burst Niagara/Trail组件结构的表现检查，以及把T05尚未接入当作永久预期的菜单丢弃检查。
- 同文件中的其他业务检查保留；物品守恒、接收失败不扣物品、会话失效、伤害结算、取消与生命周期规则不因本次清理移除。当前执行入口与覆盖边界以[构建与测试说明](../../../docs/BUILD_AND_TEST.md)为准；此前票据中的数量和运行记录只属于当时版本。
- Standards：按编码规范第6.1节执行已获授权的删除，不新增回归测试或专用检查器。Spec：本次仅调整工程检查范围，不改变玩法与用户功能验收要求。
- 静态检查：`git diff --check`通过（退出0）；PowerShell `Parser.ParseFile`解析`Tools/Build/Invoke-Wizard.ps1`无语法错误；相关测试/脚本中已无被删用例、入口和视觉辅助符号，文档本地链接检查通过。源码当前保留19个自动化用例声明、3个Smoke入口，此为静态清单，不是运行结果。
- 当前用户编辑器仍打开（PID234936），本轮不关闭用户会话；未执行构建、自动化、PIE、Cook或打包，不沿用历史通过结果作为当前版本已运行证据。未提交或推送。
