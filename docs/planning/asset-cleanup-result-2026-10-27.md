# 展示内容清理结果

日期：2026-10-27。状态：第一轮展示内容清理已完成，并完成清理后验证。

## 1. 实际变更

- 清理 412 个展示相关包，其中包含 15 张展示地图及归属这些地图的外部 Actor/Object 配套包。
- 减少 825,766,019 字节，约 787.51 MiB。
- 仅移除清单关联的 210 个空目录；没有递归删除仍有文件的目录。
- Content 从 2,523 个文件、约 3.16 GiB，减少至 2,111 个文件、约 2.39 GiB。
- 已注册包从 2,195 个减少至 1,783 个。
- 法师、法杖、敌人、技能效果、场景核心资源、当前 TopDown 模板和音效源包保留。
- 游戏 C++、配置、第三方插件源码及保留资产的资源路径没有因本次清理而修改。

操作通过 UE5.4 的 EditorAssetSubsystem 批量删除入口执行。删除前再次检查了完整候选组的保存引用；没有直接在文件管理器中批量删除 uasset。

预检清单见 [清理前候选](../../.scratch/wizard-demo/asset-cleanup-dry-run.json)，实际证据汇总见 [执行结果 JSON](../../.scratch/wizard-demo/asset-cleanup-result.json)。

## 2. 备份与版本管理

- 完整 Content 备份目录：[20261027_showcase_2968e5e1](<D:/Game project(unreal)/Wizard_AssetBackups/20261027_showcase_2968e5e1>)。
- [备份 ZIP](<D:/Game project(unreal)/Wizard_AssetBackups/20261027_showcase_2968e5e1/content-before-cleanup.zip>) 包含全部 2,523 个原文件，完整性检查通过；同目录 content-manifest.json 保存各文件 SHA-256。
- [恢复脚本](<D:/Game project(unreal)/Wizard_AssetBackups/20261027_showcase_2968e5e1/restore_removed_assets.py>) 只恢复本次剔除文件，不覆盖已有不同内容；其 dry-run 已确认可恢复 412 个文件。
- 本地 Git/LFS 已初始化；代码与配置基线为 `64f571c`，展示资源没有进入该基线的二进制历史。保留资产在清理后加入 LFS。
- 学习目录、编辑器/IDE 缓存、构建输出及 Saved/Cooked 均由 Git 忽略。

需要恢复时先关闭编辑器，再运行备份目录中的 restore_removed_assets.py；先使用 `--dry-run` 查看计划。恢复后重新扫描资产并验证关卡。此步骤尚未执行，当前保持清理后的状态。

## 3. 验证结果

| 检查 | 结果 |
| --- | --- |
| 文件清单 | 实际删除恰好 412 个；误删、漏删及新增文件均为 0 |
| 保留文件 SHA-256 | 清理后及 PIE/Cook 后两次检查，2,111 个保留文件均与完整备份一致 |
| 保存依赖 | 对剩余 1,783 个已注册包重新读取依赖，指向已删除包的依赖为 0 |
| 法师动画 | 33/33 可重新加载 |
| 角色/敌人网格 | 法师、Slime、Beholder 共 3/3 可加载 |
| 法杖 | 3/3 可加载 |
| 关键 Niagara 效果 | 普攻及六类技能候选共 7/7 可加载 |
| 默认地图 | TopDownMap 正常打开；没有未保存资产 |
| PIE | 正常启动、创建玩家 Pawn，移动测试后正常停止 |
| 位移验证 | 通过 Pawn.AddMovementInput 产生约 375.95 厘米实际位移；不是鼠标输入端到端测试 |
| Windows Cook | 当前启动地图 Cook 返回 0，处理 536 个包，摘要为 0 error(s)、310 warning(s) |
| Git LFS | 2,110 个二进制资产已纳入 LFS，`git lfs fsck` 通过 |

Cook 的 310 条唯一告警来自此前已确认不兼容的原音效包：RPG_Fantasy_SFX 100 个、UltimateUIMenusSFX 210 个。本轮没有导入恢复音频或剔除这些源包，相关工作见 [音效恢复验证](audio-recovery-2026-10-27.md)。

Cook 日志、文件核对记录和崩溃摘要保存在完整备份目录。该 Cook 检查针对当前 TopDownMap，不代表尚未制作的最终 Demo 已完成打包。

## 4. 操作期间的桥接插件异常

批量删除后，UE-MCP 的反射调用在结果序列化阶段触发访问异常，调用栈位于 `MCPFunctionCall::WriteOutputs`、`HandlerFunctionCall.h:210`，编辑器退出；桥接请求随后超时。

没有重试删除。先通过文件清单和哈希确认实际落盘结果，随后确认编辑器重新运行于 TopDownMap，并完成上述重新加载、PIE 和 Cook 验证。没有修改插件源码，也没有发现保留文件损坏。

后续销毁对象的维护操作优先使用专用资产接口或独立命令行流程，避免通过该版本反射桥接回传已销毁对象集合。

## 5. 后续边界

本轮仅清理展示内容。备用怪物、未选特效、重复季节场景、StarterContent 和模板角色的进一步裁剪，安排在核心表现与运行关卡确定后；音效迁移、项目自有内容目录及最终 Cook 规则另行实施。
