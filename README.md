# Wizard

基于 **Unreal Engine 5.4** 的 Windows 单机俯视角法师战斗 Demo，用于展示 C++ 业务设计、Gameplay Ability System（GAS）、CommonUI、Enhanced Input 和数据驱动配置的实际接入。

玩家在营地配置技能、购买装备，进入战斗区挑战三波敌人，收集战利品后回营地交易与换装，再次挑战。核心业务使用 C++；蓝图与资产承担数据配置、资源引用、动画表现和 UI 布局。

## 玩法与功能

| 系统       | 内容                                                                                       |
| ---------- | ------------------------------------------------------------------------------------------ |
| 战斗与技能 | 鼠标瞄准、单发/连续普攻；火球、冰箭、雷击、闪现、护盾、治疗六个候选技能，四个可配置技能槽  |
| 敌人与挑战 | 史莱姆近战追击、观察者远程投射物攻击；行为树与 GAS 攻击；三波挑战、结界、胜败结算与回营地  |
| 背包与掉落 | 20 格背包，拖拽交换/合并、拆分、丢弃、药水使用；金币与物品自动拾取，容量不足时保留剩余掉落 |
| 装备与交易 | 帽子、衣服、靴子、法杖四槽装备及属性加成；商人交互、商品分类、数量选择、购买与出售         |
| 存档       | 有效持久状态变更后自动保存金币、背包、装备与技能搭配；读档回营地，提供校验与备份恢复       |
| 战斗表现   | 出手/命中/状态/死亡特效与音效、结算飘字、分级震屏、短顿帧、玩家死亡动画                    |

当前为开发中的演示工程。上述内容已有工程实现，**完整玩法与成品验收尚未完成**；各项状态以[本地任务票据](.scratch/wizard-demo/plan.md)为准。

## 环境

- Windows x64。
- Unreal Engine **5.4**，已有工程验证使用 **5.4.4**。
- Visual Studio 2022，安装 C++ 游戏开发工作负载、MSVC v143 和 Windows SDK。已验证工具链为 MSVC 14.38.33145（安装目录 14.38.33130）、Windows SDK 10.0.19041.0。
- PowerShell **7+**，用于执行统一构建脚本。
- Git 与 **Git LFS**，用于获取 `.uasset`、`.umap` 等二进制资源。

项目已配置 GAS、CommonUI、Enhanced Input、Niagara 等引擎插件。仓库包含编辑器工具 `Plugins/UE_MCP_Bridge` 的源码；

## 获取与启动

在 PowerShell 中获取工程与 LFS 资源：

```powershell
git lfs install
git clone https://github.com/pureNMQ/Wizard.git
cd Wizard
git lfs pull
```

在项目根目录编译编辑器目标：

```powershell
pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor
```

脚本依次从 `-EngineRoot` 参数、`WIZARD_UE_ROOT` 环境变量、UE 安装注册记录查找引擎。如果未登记安装位置，可显式指定包含 `Engine` 子目录的 UE 安装目录：

```powershell
pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor -EngineRoot 'D:/Epic Games/UE_5.4'
```

编译成功后打开 `Wizard.uproject`，等待首次着色器编译与资源加载完成，在编辑器中点击 **Play** 试玩。编辑器与游戏默认地图均为 `/Game/Wizard/Maps/L_WizardClearing`；若当前打开其他地图，切回该地图。


## 操作

| 输入                  | 行为                                   |
| --------------------- | -------------------------------------- |
| `W` / `A` / `S` / `D` | 移动                                   |
| 鼠标位置              | 瞄准或选择目标位置                     |
| 鼠标左键              | 单击一次普攻，按住连续普攻             |
| `1` / `2` / `3` / `4` | 施放对应槽位技能，每次按下一次         |
| `Tab`                 | 打开/关闭背包                          |
| `E`                   | 靠近商人或营地炼金台时交互             |
| `Esc`                 | 关闭最上层界面，数量弹窗优先于底层页面 |
| 鼠标拖拽              | 背包换位/合并、装备穿脱、营地技能搭配  |

背包、商店和炼金台界面打开时暂停世界。技能只能在营地调整，同一技能不能重复装备；候选技能拖入槽位替换，槽位间拖拽交换，拖回候选区卸下。物理键位保存在 Enhanced Input 映射资产中。

## 建议演示路线

1. 从营地出发，在炼金台按 `E` 打开技能搭配，配置四个技能槽。
2. 进入战斗区域触发挑战，展示普攻、技能消耗与冷却，以及近战/远程两类敌人。
3. 击败敌人并拾取战利品，打开背包展示药水使用、拆分、拖拽与装备属性变化。新存档从零金币和空背包开始。
4. 完成三波或死亡后，等待结果展示并返回营地；向商人出售战利品、购买装备，再次挑战。
5. 发生金币、物品、装备或搭配的有效变化后退出，再启动检查存档恢复：回到营地、生命和法力恢复、持久状态保留。

以上是试玩与检查路线，各步骤的最终验收结果见对应票据。存档文件、备份规则与测试重置方式见[存档说明](docs/SAVE.md)。

## 构建、测试与打包

在项目根目录执行。运行前保存资源并关闭 Wizard 编辑器，脚本会检查编辑器占用，不会自动关闭窗口。

```powershell
# 编辑器目标：实际 UHT/UBT 编译
pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor

# 编译后运行 Wizard. 范围的工程自动化测试
pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Test

# 可选：按测试名称前缀筛选
pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Test -TestFilter Wizard.Inventory

# 游戏目标构建、Cook、Stage、Package 与 Archive
pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action Package
```

- 构建与测试日志、自动化报告和 `result.json` 写入 `Saved/T01/<本轮目录>/`。
- Windows Development 成品写入 `Saved/Packages/<本轮目录>/`，运行其中的 `Windows/Wizard.exe`；具体目录以脚本输出为准。
- `Saved/`、生成的二进制、缓存与本地学习资料由 Git 忽略，克隆仓库不包含本机已有游戏包或原始验证日志。
- 工程测试使用独立进程与 NullRHI；它们验证业务结果，不证明画面、音效、真实按键或渲染性能通过。

完整参数、检查边界与启动检查见[构建与测试说明](docs/BUILD_AND_TEST.md)。项目按[验证流程](docs/agents/validation.md)选择受影响检查，运行时效果由用户试玩验收。

## 代码与资源组织

| 路径                                                          | 职责                                                     |
| ------------------------------------------------------------- | -------------------------------------------------------- |
| `Source/Wizard/GAS/`、`Skills/`、`Combat/`                    | 属性与效果、能力生命周期、技能搭配、弹丸、敌人及战斗反馈 |
| `Source/Wizard/Inventory/`、`Equipment/`、`Loot/`、`Trading/` | 物品定义、库存事务、装备加成、掉落拾取与交易             |
| `Source/Wizard/Save/`、`Challenge/`、`Interaction/`           | 存档恢复、三波挑战状态与场景交互                         |
| `Source/Wizard/UI/`                                           | CommonUI 页面、菜单协调与事件驱动的状态展示              |
| `Source/Wizard/Private/Tests/`                                | 工程自动化测试                                           |
| `Content/Wizard/`                                             | 项目地图、蓝图、数据、动画、UI、音效与表现资源           |
| `Config/`、`Tools/Build/`                                     | 引擎/项目配置与统一构建入口                              |
| `SourceAssets/`、`docs/`、`.scratch/wizard-demo/`             | 素材原件及署名、系统说明、规格与任务证据                 |

玩家的 ASC、属性集、背包、装备、技能搭配与存档服务由 `AWizardPlayerState` 持有；角色负责世界中的移动与表现，控制器负责输入和界面协调。UI 通过业务接口提交操作并订阅状态变化，数值和物品状态由业务层维护。

GAS 管理能力授予、激活、消耗、冷却、结算与取消，GameplayCue 承担效果表现。物品与交易先校验再整体提交；存档使用稳定定义 ID 与数据快照，加载时统一校验和重建装备效果。新增已有行为类型的装备、技能或掉落条目，通过配置定义及资源引用扩展。

## 素材与署名

角色、环境、特效及音效包含第三方素材，来源与尚待核对的许可统一记录在[素材清单](docs/ASSET_SOURCES.md)。Game-icons.net 图标的作者、原始链接与 CC BY 3.0 署名见[图标署名](SourceAssets/Icons/GameIcons/ATTRIBUTION.md)。生成的怪物素材图标见[生成记录](SourceAssets/Icons/GeneratedMonsterMaterials/GENERATION.md)。

源码与第三方素材的使用、分发条件分别核对；最终素材许可与交付清单仍由 T10 跟踪。
