# Wizard：UE5.4 俯视角 RPG Demo

用于求职展示的 Windows 单机项目，重点是 C++ 代码质量、数据驱动，以及 GAS、CommonUI、Enhanced Input 的正确使用。

## 当前基准状态

当前版本是规格与素材基线：保留原始 C++ 模板、清理后的核心素材和开发规划。游戏核心系统尚未实现。

- [规格 v1.0](.scratch/wizard-demo/spec.md) 已确认，Git 标签为 `spec-v1.0`。
- 当前可运行关卡仍使用模板点击移动；WASD、四槽技能、背包、商店、装备和存档属于后续实施范围。
- Enhanced Input 已在模板中使用；GAS 和 CommonUI 插件已启用，业务接入尚未实现。
- 第一轮已移除 412 个素材展示包，核心资源保留。
- 项目基准标签为 `baseline-2026-10-27`，不是游戏成品发布标签。

## 环境与获取

当前验证环境是 Windows、UE5.4.4。需要 Git、Git LFS，以及可构建 UE5.4 编辑器目标的 Visual Studio C++ 和 Windows SDK 工具链。

先安装并启用 Git LFS，再获取项目：

```powershell
git lfs install
git clone https://github.com/pureNMQ/Wizard.git
Set-Location Wizard
git lfs pull
git lfs fsck
```

二进制资产由 LFS 管理；GitHub 的源码 ZIP 不作为完整素材检出的替代方式。

## 构建与启动

在项目根目录运行，修改 EngineRoot 为本机的 UE5.4 路径：

```powershell
$EngineRoot = 'D:\Program Files\Epic Games\UE_5.4'
$ProjectFile = (Resolve-Path '.\Wizard.uproject').Path
& "$EngineRoot\Engine\Build\BatchFiles\Build.bat" WizardEditor Win64 Development "-Project=$ProjectFile" -WaitMutex -NoHotReloadFromIDE
& "$EngineRoot\Engine\Binaries\Win64\UnrealEditor.exe" $ProjectFile
```

默认地图为 `/Game/TopDown/Maps/TopDownMap`。进入 PIE 可运行当前模板。此处描述的是现有基准的启动方式，完整 Demo 操作将在对应功能完成后补齐。

## 项目索引

| 路径 | 内容 |
| --- | --- |
| Source/Wizard | 游戏 C++ 模块，目前主要为模板代码 |
| Content | 角色、动画、法杖、敌人、场景、效果及图标等素材 |
| Config | UE 项目配置 |
| Plugins/UE_MCP_Bridge | 第三方开发辅助桥接插件，与游戏自有代码区分 |
| .scratch/wizard-demo | 规格、任务计划及结构化检查记录 |
| docs/planning | 框架与质量草案、素材审计、恢复与清理记录 |

详细计划见 [实施计划](.scratch/wizard-demo/plan.md)，质量方向见 [工作流与质量草案](docs/planning/workflow-and-quality-draft.md)。学习笔记、个人练习和测验保存在本地忽略目录，不进入仓库。

## 基准验证与已知限制

推送前检查见 [基准检查记录](docs/planning/baseline-2026-10-27.md)。

- 本地 Git/LFS 检查、当前编辑器目标检查、保存依赖检查、PIE 启动和 Windows 迭代 Cook 通过。
- 两套原音效包在 UE5.4 中存在包格式不兼容问题，当前 Cook 有 310 条已记录告警。205 条音频已在本机恢复为 WAV，尚未导入项目；SoundCue 节点图未恢复。见 [音效恢复记录](docs/planning/audio-recovery-2026-10-27.md)。
- 开发桥接插件曾在批量删除后的反射结果回传中崩溃。编辑器及资产完整性已恢复并验证，插件源码未修改。后续对象销毁维护避免使用该版本反射桥接回传被销毁对象集合。见 [清理结果](docs/planning/asset-cleanup-result-2026-10-27.md)。
- 当前验证不代表最终 Demo 验收、干净环境完整构建或成品打包已经完成。
