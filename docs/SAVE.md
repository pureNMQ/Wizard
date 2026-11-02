# 存档与恢复

2026-11-01，T08工程实现；规格见[第8节](../.scratch/wizard-demo/spec.md)，验收与证据见[T08](../.scratch/wizard-demo/issues/08-save-and-restore.md)。运行效果和独立包重启尚待用户验收。

## 玩家操作与测试删档

金币、背包/装备、四槽技能的有效变化自动保存，无手动保存按钮。读取存档后回到营地，生命/法力按装备后的上限恢复。没有存档时使用初始搭配、零金币、20个空背包格；未发生持久变化不会创建文件。

编辑器测试目录为项目的`Saved/SaveGames/`；正式单槽文件如下：

| 文件 | 用途 |
| --- | --- |
| `WizardProgress.sav` | 最新成功快照 |
| `WizardProgress.sav.bak` | 上一成功快照；主档异常或缺失时尝试恢复 |
| `WizardProgress.sav.tmp` | 写入中间文件，不作为成功快照加载 |
| `WizardProgress.sav.rejected-<GUID>` | 恢复备份时隔离的异常主档，保留诊断依据 |

测试重新开始：先停止PIE/退出游戏，再把上述`WizardProgress.sav`及其同名前缀文件一起移出目录或删除，最后重新运行。只删除主档会尝试恢复备份。游戏运行中删文件不会清除内存状态，后续有效变化仍会保存。项目编辑器目录为`D:\Game project(unreal)\Wizard\Saved\SaveGames`；成品使用自身的`FPaths::ProjectSavedDir()`，具体路径查看启动日志“存档会话”，不假设与编辑器共用目录。

## 职责与流程

- `AWizardGameMode::RestartPlayer`首次成功占有后，使用当前挑战Actor配置的营地返回点，再启动一次保存会话。角色再次初始化不会重新读档覆盖当前进度，不修改地图布局。
- `UWizardSaveComponent`由PlayerState持有，监听库存、金币和搭配的提交事件。恢复期间不自动保存，EndPlay解除订阅。没有新增GameInstance或第二份运行时物品权威状态。
- `UWizardSaveGame`保存版本、金币、库存/装备快照和四槽技能ID。物品仅序列化稳定ItemId、实例GUID、数量与位置；定义引用为Transient，恢复时沿用AssetManager的WizardItem扫描。库存Revision写0，恢复时生成新的进程内令牌。
- `AWizardPlayerState::RestorePersistentState`先解析并校验全部字段，再一次赋值并广播；通知期间拒绝金币、物品和技能的写入重入。装备组件复用已有GE重建入口，最后恢复生命/法力。非法快照不提交任何部分。
- 不保存位置、敌人、地面掉落、挑战轮次、当前资源值、技能冷却、临时效果或GE Handle。新进程/新世界正常初始化挑战。

## 文件格式与写入取舍

采用`UGameplayStatics::SaveGameToMemory/LoadGameFromMemory`进行UE原生对象序列化。外围增加12字节Windows小端封套：uint32格式标记、uint32负载长度、uint32 CRC32。读取先限制文件大小为1MiB并核对长度/CRC，再反序列化；对象的SchemaVersion目前为1。CRC用于检测意外损坏，不提供加密或防篡改。该文件需要经WizardSaveStorage解封，不能直接交给LoadGameFromSlot。

本机UE5.4.4源码契约：`Engine/Source/Runtime/Engine/Private/GameplayStatics.cpp`中的SaveGameToMemory/LoadGameFromMemory负责对象与字节转换。正常非Transient反射字段会被序列化，不能假定仅加SaveGame标记才写入。

首版选择同步小快照：在已提交状态事件内立即完成写入；库存与余额同次提交的两条通知通过完整字节快照去重。没有异步队列、未完成回调或旧快照晚到覆盖的窗口；不宣称排队等于落盘。后续数据量增长或试玩出现可归因的卡顿时，再评估串行异步写入及退出等待。

写入先完整关闭`.tmp`，再复制现有主档到`.bak`，最后移动临时文件替换主档。替换并非全流程单个原子操作：中断若留下缺失/损坏主档，可恢复备份；这不保证强制断电时最新一次操作必然保留。替换返回成功才发布Saved状态。失败保留运行状态、报告WriteFailed，下一次变更或显式SaveNow重试；正常EndPlay对未成功的最终状态再尝试一次。

## 异常恢复

- 主档通过文件、版本和完整业务校验才恢复；失败时尝试备份，不能局部加载有效格子并静默丢弃其余物品。
- 备份有效时隔离异常主档为`.rejected-<GUID>`，明确提示已恢复备份；之后允许继续保存。
- 无有效备份，或无法隔离异常主档时：保留原件，设置LoadBlocked并禁止本会话覆盖。玩家可继续操作，但提示“本次进度不会保存”；需退出后保留/移走异常文件再重新启动。
- 写入失败、恢复备份和加载阻塞通过现有HUD公告显示实际结果；正常保存不加常驻文案。日志保留路径及解码、版本或业务校验原因。

## 用户运行验收

1. 清档启动，确认营地位置、空背包与初始技能；获取金币/物品、买卖、拆分、换装并调整技能槽（包含空槽），记录余额和数量。
2. 停止再启动PIE，确认金币、格子位置/数量、装备及四槽搭配一致，属性加成只有一份，生命/法力全满。
3. 在挑战中退出再启动，确认回营地、挑战重新开始，原地面掉落不恢复；重复初始化和再次挑战不重复加成。
4. 正式成品完成一次退出重启往返，确认定义与图标可加载；PIE/临时世界测试不能替代此项。
5. 留意操作保存时是否有可感知停顿；异常反馈实际可读性待用户反馈。工程自动化已模拟写失败、损坏/截断/不兼容数据和备份恢复，不要求破坏正式进度以复验。

工程复跑：先`pwsh -NoProfile -File ./Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor`，再`-Action Test -TestFilter Wizard.Save`；跨系统回归用`-Action Test`。测试使用随机WizardTest槽名并仅清理自身文件，不读写WizardProgress。
