# Wizard 性能复现与证据工具

这些工具读取本地采样文件，不修改游戏业务、蓝图、地图或编辑器。`Export-WizardTrace.ps1` 以 Unreal Insights 的 `-NoUI` 模式分析已停止录制的本地 trace，写出 CSV 与日志，不连接编辑器。实际采样由 UE-MCP 的控制台命令和 trace 启停接口触发。

## 固定条件采样

每轮记录编辑器版本、是否冷启动、地图、镜头、PIE 模式、是否攻击、前台焦点、帧率上限，以及后台 CPU 节流和 `Slate.bAllowThrottling` 的原值。先采默认条件，再一次只改变一个因素；临时值在结束后恢复，不将临时取消节流写成根因修复。记录着色器编译、资源加载及 GPU timeout 日志，避免把不同启动阶段混作同一种卡顿。

通过 MCP 执行下列独立控制台命令。`STARTFILE` 只设置下一轮文件名，必须随后执行 `START`：

```text
csv.CompressionMode 0
CsvProfile STARTFILE=Wizard-PIE-UniqueName.csv
CsvProfile START
```

保持既定行为约 10 秒墙钟时间，再执行 `CsvProfile STOP`，等待日志 `Capture Ended. Writing CSV to file`。默认文件位于 `Saved/Profiling/CSV/`。先记录压缩设置原值，结束恢复。不要用 `FRAMES=600` 代替固定墙钟窗口：严重停顿时会显著延长采样。启动卡顿应在按 Play 前开始采样；Idle、攻击与冷启动分别记录。

使用 MCP trace 接口启用 CPU、GPU、Frame、File、LoadTime 通道，在目标操作前开始，复现后停止并记录实际 `.utrace` 路径。同一严重样本优先同步 CSV 和 trace；等写入结束后再离线分析，不把仍在写入的文件当完整证据。

## CSV 判定

在项目根目录运行 PowerShell 7：

```powershell
pwsh -NoProfile -File Tools/Performance/Measure-WizardCsv.ps1 -Path 'Saved/Profiling/CSV/Wizard-PIE-UniqueName.csv' -OutputPath 'Saved/T02/Performance/summary.json'
```

输出包括帧数、实际时长、最大帧、P95、平均帧、严格大于 100ms/500ms 的次数，以及长帧的源文件物理行号和 GT/RT/GPU 指标。默认要求至少 8 秒有效采样，两个阈值允许次数均为零；可通过 `-MinimumDurationSeconds`、`-MaxFramesOver100Ms`、`-MaxFramesOver500Ms` 显式调整。

| 退出码 | 含义 |
| --- | --- |
| 0 | 输入完整，满足本次给定阈值 |
| 1 | 输入可解析，但时长或停顿阈值失败 |
| 2 | 文件缺失、格式错误或采样未完整结束，不能判为通过 |

UE5.4 尾部 metadata 的 Commandline 引号不遵循标准 CSV；分析器按最终 `EVENTS` 表头划定数据区，将 metadata 单独处理，不静默忽略损坏的数据行。

原生 CSV `FrameTime` 来自相邻 EndFrame 的平台时钟间隔，不是游戏 `DeltaSeconds`，不会被 fixed delta 直接替换。游戏反射接口读取的 `DeltaSeconds`、平均 FPS 和一次流畅体验不能替代真实帧间隔；GT/RT 时间可能扣除了等待或属于异步完成的测量，也不能与墙钟帧简单相减得出唯一根因。STOP 请求遇到阻塞可能晚执行，因此实际时长应与 CSV metadata 和日志交叉核对，不强行裁成请求的 10 秒。

## 无界面 trace 导出

```powershell
pwsh -NoProfile -File Tools/Performance/Export-WizardTrace.ps1 -TracePath 'Saved/T02/Performance/Example.utrace' -OutputDirectory 'Saved/T02/Performance/Trace-Example'
```

输出目录必须是新目录。工具从本机 UE5.4 安装记录定位 Insights，也可用 `-InsightsExecutable` 指定。导出 `threads.csv`、`timers.csv`、`events.csv`、`statistics.csv`、执行日志及 `export-result.json`；事件时间单位为秒。默认事件线程筛选 `GameThread,Render*,RHIThread`，需要追踪后台资源分配时加 `-EventThreads '*'`。全线程事件导出可能显著膨胀，应保留原始 trace，并按问题缩小导出范围。

工具使用引擎原生 `-OpenTraceFile`、`-NoUI`、`-AutoQuit`、`-ExecOnAnalysisCompleteCmd=@=...` 批处理接口，显式等待进程结束并检查导出文件。退出 0 表示导出进程及文件检查通过，不表示性能通过；退出 2 表示启动、分析或导出检查失败。

## 结论边界

同一长帧应结合线程、起止时间和嵌套 Depth 建立等待链，不能只按 timer 总时长排名猜根因。例如冷启动严重样本曾显示 GT 等待 RT/RHI，RT 等待后台纹理分配；17.3819 秒分配任务中，`CreateCommittedResource` 子区间累计约 17.3800 秒。该计时范围还包含资源包装、驻留跟踪和 Aftermath 资源注册，不能直接称为裸驱动 API 耗时。

这类证据不意味着所有大于 100ms 的启动帧都同根因。一个未复现严重停顿的 trace，只能说明该轮未捕获同一症状；关闭某项后的一次通过也不能代替相同冷启动条件的重复对照。正式性能结论、完整路径和已知限制写回对应票据，本说明不维护第二份验收状态。
