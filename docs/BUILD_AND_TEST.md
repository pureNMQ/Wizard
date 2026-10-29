# 构建、测试与最小 Windows 包

## 使用前

在项目根目录用 PowerShell 7 执行。保存并关闭当前 Wizard 编辑器，避免 DLL 被占用、Live Coding 状态或未保存资产影响结果。脚本只检查本项目编辑器，不自动关闭用户窗口。

统一入口为 `Tools/Build/Invoke-Wizard.ps1`。引擎位置依次从 `-EngineRoot` 参数、`WIZARD_UE_ROOT` 环境变量、本机 UE EngineAssociation 注册表安装记录读取；无需把本机路径写入游戏代码。

## 常用命令

```powershell
# 编译编辑器目标，包括项目自动化检查。
pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor

# 运行 Wizard. 下的环境和战斗测试，逐项核对源码声明与真实报告。
pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Test

# 可按名称前缀筛选；例如只运行环境检查。
pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Test -TestFilter Wizard.Environment

# 编译游戏、Cook 当前地图、汇总文件、打包并归档 Development 成品。
pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Package

# 对上一步打印的具体成品目录进行启动/移动检查；替换为实际路径。
pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Smoke -PackageRoot './Saved/Packages/<本轮目录>'

# 对当前版本的独立包检查真实弹丸、GE、HUD、连射停止、暂停和角色重建。
pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Smoke -CombatSmoke -PackageRoot './Saved/Packages/<本轮目录>'

# 对当前独立包检查六技能、取消路径与Alchemy搭配。
pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Smoke -SkillSmoke -PackageRoot './Saved/Packages/<本轮目录>'

# 顺序执行上述四步；任一步失败即停止。
pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action All
```

本机引擎没有登记安装位置时，给任一命令追加 `-EngineRoot '<UE安装目录>'`。此目录下应包含 `Engine/Build/BatchFiles/Build.bat`。

`Test` 使用独立的 UnrealEditor-Cmd 进程和 NullRHI，不占用交互编辑器，不加载 UE-MCP 桥接。先运行 BuildEditor，使新测试进入模块；默认筛选 `Wizard.`，也可用 `-TestFilter Wizard.Combat` 等名称前缀。脚本从 `Source/Wizard/Private/Tests` 的 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` 字面量收集筛选范围内的预期测试；不匹配任何源码测试、缺失报告、漏跑预期测试、失败/未运行测试以及非零退出码都视为失败。新增复杂或参数化宏时需同步扩展发现规则。`result.json` 保存筛选器、预期名称与实际报告位置，旧 DLL 漏编新增测试不会被当作成功。

## 输出与运行

- 每次检查在 `Saved/T01/<时间-动作-编号>/` 写控制台日志、引擎日志、自动化报告和 `result.json`；失败时保留已完成步骤和错误。
- 每次 Package 使用新的 `Saved/Packages/<本轮目录>/`，避免旧文件掩盖打包缺失。脚本不会清理以前的包。
- 正常试玩双击归档中的 Windows 启动程序。也可运行实际的 `Windows/Wizard/Binaries/Win64/Wizard.exe`；目录层级以本轮打印结果为准。
- Smoke 隐藏启动 1280×720 游戏窗口，避免抢占用户输入；检查启动地图、GameMode、已占有的玩家、导航移动与落地，然后正常退出。实际按键验收另开可见成品。可用 `-SmokeSeconds 20` 延长观察时间。
- Smoke 同时请求引擎原生 HighResShot 截图，保存位置由成品日志中的 `High resolution screenshot saved as` 行给出；必要时复制到本轮证据目录。截图用于补充视觉检查，脚本的通过条件仍以启动/移动结果为准。
- 只有显式执行开发命令 `Wizard.SmokeTest` 才会触发该检查和退出；普通启动不自动运行。Shipping/Test 构建不注册该命令。
- `Smoke -CombatSmoke` 改为调用 `Wizard.CombatSmokeTest`，在当前真实世界生成临时高血量目标，检查单发命中、GE 扣血、法力不变、HUD 属性事件、连射/松开、菜单暂停/关闭后无遗留连射，以及角色替换和幂等能力授予。弹丸类和拖尾系统的预期值从当前GA配置及弹丸类默认组件读取，检查实际生成类、Niagara激活与停止；异步出手的观察方式见下述蒙太奇检查。测试会替换玩家 Pawn，结束清理目标并正常退出独立游戏；PIE 手动调用命令时保留编辑器。该检查通过公开业务接口发起攻击，不能替代物理输入、鼠标瞄准、CommonUI 实际 Esc/焦点和视觉验收。命中失败也可能说明测试目标前存在场景遮挡，需查日志定位。
- 普攻蒙太奇迁移后，CombatSmoke的首发改为验证真实蒙太奇出手通知：Start同步尚无弹丸，立即普通Release仍会在Notify后只发一发，重复事件不增发，最终GA/蒙太奇/Task均结束。使用世界Spawn与组件移动事件观察短飞行，不依赖轮询恰好捕获；命中GC预期从当前GE和CueSet的Burst资源读取。另检查出手前显式取消、UI、死亡、真实Montage_Stop打断，以及运行实例放慢10倍后冷却先结束仍能续发；测试不改CDO或资源。后台节流会让真实墙钟与世界模拟进度不一致，PIE测试需记录并临时排除该条件，结束恢复原值。最终成品包运行与编辑器测试分别留证。

## 检查内容与边界

| 检查 | 能证明什么 | 未覆盖什么 |
| --- | --- | --- |
| StartupConfiguration | 游戏/编辑器默认地图、地图包和 GameMode/Pawn/Controller 类型可加载 | 真实按键、技能或 UI 路由 |
| SceneStructure | 已保存地图有出生点、导航范围、六个业务标记及有碰撞的地面 | 地图观感、完整通行路线和全部碰撞 |
| Packaged Smoke | 真实 Development 成品加载当前场景、生成玩家，导航请求产生实际位移且保持落地，随后退出；日志无桥接加载 | 鼠标/键盘端到端、存档往返、完整玩法和性能基准 |

编辑器目标与 Game 目标分别构建。EngineSettings 仅作为编辑器测试依赖；UE_MCP_Bridge 的项目引用限定 Editor。成品边界通过包内文件与启动日志一并核对，不能只依据描述文件下结论。

CommonUI viewport 路由的既有告警由 T02 接入处理，本票据不以关闭告警代替修复。素材来源与许可待办见 [素材清单](ASSET_SOURCES.md)，最终 R30/R31/R32 交付由 T10 完成。

`Smoke -SkillSmoke` 使用`Wizard.SkillsSmokeTest`，日志名为`packaged-skills-smoke.log`；与`-CombatSmoke`互斥。真实Notify推进火球/冰箭命中、冰减速、雷击范围、闪现、护盾、治疗和扣蓝/冷却，再验证显式取消、UI、死亡、换Avatar的出手前取消，最后检查Alchemy范围、交换、重复限制及菜单暂停。开发命令临时修改运行时数值/搭配并生成目标，退出前清理，不改保存地图或配置；它不替代物理1234/E/Esc与拖拽验收。

`-SmokeNullRHI` 可用于排除同时试玩时的GPU资源竞争，仅验证业务、动画通知及碰撞；`result.json`的`renderingEnabled=false`明确标记此边界。该模式不能证明粒子像素、实际声音、截图或渲染性能通过；资源表现仍需正常渲染的成品/PIE及实际试玩证据。

本机执行结果与具体证据路径记录在 [T01](../.scratch/wizard-demo/issues/01-environment-and-quality.md)，以实际结果为准；此文档不把未来玩法描述为已实现。
