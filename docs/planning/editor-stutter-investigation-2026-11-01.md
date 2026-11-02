# UE 编辑器播放卡顿调查（2026-11-01）

状态：调查中。已由零丢失Windows记录确认：部分D3D12资源创建阻塞在MakeResident/分页同步等待。电脑重启后曾实测恢复，随后复发；驻留等待为何变慢及进入异常状态的触发源仍未确认，未宣称修复。

## 范围与结论

用户报告编辑器播放、新对象出现时严重卡顿，常伴随“准备编译着色器”，重启电脑恢复；怀疑构建、测试或 MCP Live Coding。本次明确授权短时间 PIE 诊断、临时关闭 NVIDIA 覆盖层及确认资产保存后重启 UE。

环境：UE 5.4.4、RTX 4060 Laptop 8GB、驱动 581.42。原编辑器 PID 182580，重启后分别为 252300、260068。系统最近启动于 2026-10-31 23:19:51，本轮未重启电脑。

**直接瓶颈是 D3D12 GPU 资源创建路径的长停顿，并非本次采样中的着色器编译。** 调用链包括 `FRDGBuilder::AllocatePooledTextures`、`FD3D12PoolAllocator::CreateNewPool`、`CreateCommittedResource`，游戏线程通过 `GameThreadWaitForTask` 等待渲染工作。

本机引擎 `D3D12Resources.cpp` 中，`CreateCommittedResource` 计时范围包括驱动调用、UE 资源包装、命名和驻留追踪。因此尚不能区分驱动内部阻塞与 UE 资源管理阻塞，不能认定为已确认的 NVIDIA 驱动缺陷。

## 实测对照

单位为毫秒。长帧来自 CPU `Frame` 事件；资源创建为同名事件最大值。不同轮记录长度不同，只判断长停顿是否仍存在；不将跨线程累计时间相加当成单帧时间。

| 采样 | 最大长帧 | 最大单次资源创建 | 结果 |
| --- | ---: | ---: | --- |
| baseline：原故障编辑器开始 PIE，营地静止 | 6774.334 | 2772.999 | 复现 |
| repeat：同进程再次 PIE，传送至战斗区生成敌人 | 4296.510 | 1912.499 | 复现；汇总未区分开场与敌人生成的峰值 |
| restarted：覆盖层仍启用，仅重启 UE | 6983.127 | 450.346 | 未消除 |
| no-overlay：NVIDIA 设置禁用信息浮窗后重启 UE | 6638.186 | 368.440 | 未消除 |
| single-rhi：临时 `r.RHIThread.Enable 0` | 3409.686 | 338.333 | 未消除，不能作为修复 |

五轮 `FShaderCompilingManager::BlockOnShaderMapCompletion` 最大仅 0.019～0.036 ms；有图形 PSO 创建事件的四轮中，最大不超过 3.516 ms，single-rhi 没有该事件，不能解释为真实 0 ms。观察到的编译状态快照 shaders/assets 均为 0，没有活动的 ShaderCompileWorker。

一次性独立 D3D12 探针选择 NVIDIA 适配器，各创建/释放20次16MB上传缓冲、2048² RGBA16F渲染目标、同尺寸UAV。三类最大创建时间1.317、4.177、4.645 ms，退出0。该探针没有复现秒级停顿，但不等价于 UE 多线程负载、驻留状态和驱动配置，不能据此排除驱动。只用本机 MSVC 编译探针，没有构建项目；临时源码、目标文件、可执行文件清理，只保留测量结果。

系统可用物理内存快照约6GB；GPU总占用约5.1～6.7GB，仍有余量。未连续采集峰值，不能排除瞬时显存预算/驻留压力；没有遗留构建、测试编辑器或编译器进程。

## 对用户所疑操作的核对

- 原故障会话日志仅有 Live Coding 初始化，没有应用补丁/重实例化记录。不能因存在 LiveCodingConsole 就认定执行过编译，更不能认定补丁积累是本次根因。
- 最近 `Saved/T01/20261101-001055-Test-030560/result.json` 明确为 `-NullRHI -DisablePlugins=UE_MCP_Bridge`，退出0，24项通过。这轮测试不使用正常D3D12路径，也未加载桥接；不能排除其他历史操作或间接影响。
- 当前标准构建/测试脚本要求编辑器关闭，测试采用独立 UnrealEditor-Cmd，与文档一致。
- 原故障编辑器运行值 NvAfterMath、GPUCrashDebugging、DRED 均为0。日志 `Aftermath initialized` 不能单独证明最大GPU追踪生效。用户已有未跟踪文件 `Config/DefaultDefaultEngine.ini` 含 NvAfterMath=1，本轮保留未修改，未把该非标准文件名中的值当成生效配置。
- MCP提示客户端1.4.10可更新至1.4.11，桥接缺少48个所广告的StateTree方法。本轮未升级，未将版本差异认定为原因。
- 最初尝试结束覆盖层进程后，它被后台服务拉起。因此 restarted 是“保留覆盖层仅重启UE”的对照。随后在 NVIDIA 设置中确认“信息浮窗切换开关关”后执行 no-overlay；新UE仍加载 nvspcap64.dll，故仅证明禁用浮窗没有解决，不能宣称已排除所有 NVIDIA 注入影响。

## 复跑与证据

原始 `.utrace`、`*-stats.csv`、分析日志、`comparison.json`、`allocation-probe.csv` 位于忽略目录 `Saved/Profiling/Stutter-20261101/`。本报告保留关键步骤和数值，原始文件不是唯一结论来源。

复跑步骤：检查未保存资产和后台进程 → 临时将 `/Script/UnrealEd.Default__EditorPerformanceSettings.bThrottleCPUWhenNotForeground` 置false → MCP `editor.start_trace` 开启cpu/frame/gpu，指定绝对路径 → 开始PIE → 采集 `get_frame_timing` → 需要时传送玩家至(1600,0,110)生成敌人 → 停止PIE及trace → 导出统计 → 恢复设置。后台节流原先为true，最初3FPS读数属于节流，不作为故障证据。

```text
UnrealInsights.exe -OpenTraceFile="<绝对路径>/baseline.utrace" -AutoQuit -NoUI -ExecOnAnalysisCompleteCmd="TimingInsights.ExportTimerStatistics baseline-stats.csv"
```

本机工具的相对导出路径实际落在引擎 Binaries/Win64；本轮已复制到证据目录并清理引擎目录中的本轮副本。

已执行捕获回放判定：

```powershell
$runs = Get-Content Saved/Profiling/Stutter-20261101/comparison.json -Raw | ConvertFrom-Json
$failures = @($runs | Where-Object { $_.MaxFrameMs -ge 1000 -and $_.MaxAllocationMs -ge 100 })
if ($failures.Count -gt 0) { Write-Output "REPRODUCED: $($failures.Count)/5 captures contain a >=1000ms frame and >=100ms GPU resource allocation."; exit 2 }
```

实际输出 `REPRODUCED: 5/5 captures contain a >=1000ms frame and >=100ms GPU resource allocation.`，退出2（故障检出）。此命令回放已有捕获，不能检测未来复发；新状态必须重新采样。

## 仍需验证

1. 已于本日01:05前后取得电脑重启后的正常基线，详见下节；首轮缺少正常基线的限制已解除。
2. 已完成实际增量编译/链接、单次NullRHI测试和无内容变更的单次Live Coding对照，均未触发原故障。未验证长期迭代积累、结构性代码变化、正常RHI测试或Cook/打包的触发关系。
3. 若仍不能关联触发操作，采集停顿时的原生线程栈/GPU驻留事件，拆开驱动调用、资源命名及驻留追踪。可再做临时DX11/DX12对照，不直接落盘为正式配置。

本轮没有升级/回退驱动、清除DDC/着色器缓存或重启显卡驱动，没有证据支持直接采用这些操作作为修复。

## 首轮恢复与双轴检查

PIE/trace停止，独立RHI线程恢复启用，后台CPU节流恢复true，NVIDIA信息浮窗恢复原先启用状态。每次关闭编辑器前内容/地图dirty均为0。未改游戏源码、地图或资产，没有项目构建、自动化套件、Cook、打包、提交或推送。

Standards：遵守验证流程的性能对照及本次用户授权，保留用户改动；无可靠底层诱因测试接缝，不新增长期测试。Spec：本次属于R29/R33/R34交付风险排查，未改变玩法范围，故障仍未解决，不提升功能或性能验收状态。

参考：[Epic Insights 导出说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-the-timers-and-counters-tabs-in-unreal-insights-for-unreal-engine)、[AMD D3D12 分配器说明](https://gpuopen-librariesandsdks.github.io/D3D12MemoryAllocator/html/optimal_allocation.html)。后者说明资源创建可能在驱动中阻塞其他D3D12调用，只支持调查方向，不证明本机根因。

## 重启电脑后的分步对照（01:04～01:11）

系统启动时间为2026-11-01 00:56:55，首个项目编辑器PID22236，01:00:31启动。基线采样前未执行构建、测试或Live Coding；当前日志没有用户先行PIE/补丁记录，最近测试目录仍是重启前的00:10。沿用原地图、正常D3D12、NVIDIA浮窗与独立RHI线程；只临时关闭后台节流以测量。

每次操作为开始PIE→传送玩家到(1600,0,110)触发敌人生成→短暂停留→停止，使用相同Insights导出方式。记录窗口长度不完全相等，不比较整体平均FPS；指标用于检出先前的秒级停顿。此轮没有重现原故障。

| 状态（按执行顺序） | 最大CPU长帧ms（含PIE启停） | 最大单次GPU资源创建ms |
| --- | ---: | ---: |
| clean：重启后首次PIE | 485.559 | 28.883 |
| clean-repeat：同进程第二次PIE | 228.386 | 4.441 |
| after-build：实际增量编译/链接后重新打开编辑器 | 321.171 | 5.655 |
| after-test：独立NullRHI测试后重新打开编辑器 | 316.941 | 3.892 |
| after-livecoding：MCP触发实际Live Coding补丁后，同进程复测 | 286.811 | 3.752 |
| vram-plus1g：同进程，独立探针额外占用1GB显存 | 259.161 | 4.542 |

首次进入PIE采样到92.9ms帧；生成敌人后单帧采样16.667ms、约60FPS。未据此宣称所有运行帧无卡顿；上表包含初始化与销毁峰值。

### 工程动作及边界

- 标准命令 `pwsh -NoProfile -File Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor` 首次仅up to date（退出0），证据 `Saved/T01/20261101-010554-BuildEditor-8aca8b`，不当成实际编译对照。
- 随后只更新 `Source/Wizard/Wizard.cpp` 的修改时间以使现有代码重新编译，内容未改，结束恢复原修改时间。相同标准命令实际完成 `Module.Wizard.3.cpp` 编译、Wizard模块库/DLL链接、WriteMetadata，退出0；`Saved/T01/20261101-010611-BuildEditor-807ac7`。这是编辑器关闭时的实际增量编译，不是全工程清理重编。
- 标准命令 `pwsh -NoProfile -File Tools/Build/Invoke-Wizard.ps1 -Action Test` 使用NullRHI、禁用桥接，24成功/0失败/0未运行，退出0；`Saved/T01/20261101-010740-Test-3ff010`。
- 只更新同一源码修改时间后，调用MCP `editor.hot_reload`。已读桥接实现，Windows路径实际调用 `ILiveCodingModule::Compile`。日志确认编译一个文件、链接patch_0、`Patch creation ... successful`、`Live coding succeeded`、`Reload/Re-instancing Complete: No object changes detected`。恢复源码时间。不能只用“compile triggered”返回值宣称成功。
- LiveCodingConsole同时输出多条 `Cannot find image section .voltbl`，但最终链接和补丁应用成功；已保存日志，未将此错误文字直接认定为卡顿原因。这轮未测试反射结构变更、对象替换或多次补丁积累。
- 独立本机D3D12探针创建16个64MB默认堆缓冲并调用MakeResident成功，45秒后自动退出。采样GPU总占用约6605～6615MiB，仍没有原故障。故障时DWM显存快照约1715MiB，重启后约804MiB；人工额外1GB只能测试粗略总占用，不能模拟DWM的资源形态、碎片、驻留历史或驱动内部状态，因此不能排除这些因素。

### 证据、判定及现场恢复

新增原始证据在 `Saved/Profiling/Stutter-20261101-Reboot/`：六份trace及统计、comparison.json、livecoding.log、after-livecoding-editor.log、GPU采样和探针退出记录。按首轮判据“CPU长帧≥1000ms且单次资源创建≥100ms”，本轮0/6检出，首轮5/5检出。该差异确认重启恢复，但不能将单次对照解释成永久排除构建、测试或Live Coding。

临时探针源码/obj/exe清理；没有增加正式工具或长期测试，没有修改游戏源码内容、配置、地图或资产，保留用户的DefaultDefaultEngine.ini。诊断Live Coding补丁通过最后一次安全重启UE卸载，最终编辑器PID57416，PIE/trace均停止，后台CPU节流恢复true，显存探针退出释放资源。每次关闭编辑器前内容/地图dirty均为0。未Cook、打包、提交或推送。

Standards：同条件前后对照、实际编译与up to date分别记录，24项测试通过；未把单次阴性样本当成根因排除。Spec：本轮确认当前恢复，原故障触发源仍未确认，R29/R33/R34性能风险仍未关闭。后续优先在自然复发时保留现场，捕获原生线程栈/GPU驻留事件并与本轮正常基线比较；不继续无依据重复打包或叠加探针。

## 较轻复发现场（01:53起）

用户反馈开始播放、首次普攻/技能/掉落、首次打开界面时卡顿。电脑仍是00:56:55启动，当前UE PID160380于01:48:26启动；没有重启现场，没有应用诊断补丁。此间存在其他功能开发改动，本轮全部保留，没有拿不同版本的业务行为直接做归因。

当前会话日志只有Live Coding初始化，没有补丁应用。最近标准检查为01:47:10构建、01:47:48 NullRHI测试（7项成功，禁用UE_MCP_Bridge）；这只是操作时间线，不构成触发关系。没有活动的ShaderCompileWorker、构建或测试编辑器进程。

| 操作记录 | 最大CPU帧ms | 明确观测 |
| --- | ---: | --- |
| recurrence：PIE开始，传送至战斗区 | 516.954 | CreateCommittedResource最大94.065ms；正常基线约4～6ms，着色器等待0.020ms |
| first-attack：本次诊断的一发普攻，正常Release | 19.273 | 没有CreateCommittedResource事件，未复现；不能当作用户会话第一次使用 |
| repeat-attack：同PIE再次普攻 | 18.235 | 未复现 |
| first-ui：同PIE打开背包（返回True） | 757.645 | OpenInventory本身3.569ms，GameThreadWaitForTask最大752.057ms |

UI捕获继续拆分：`FRDGBuilder::AllocateTransientResources`最大131.142ms；随后RenderThread在`WaitForGatherDynamicMeshElements`等待745.108ms，Foreground Worker #0在`OcclusionCullPipe / SyncPoint_Wait`等待745.209ms。本段没有CreateCommittedResource事件。因此不能把UI的整段758ms归因到已确认的某次CommittedResource调用，更不能把任务等待计时当成驱动内部调用栈。需要系统调度/驻留或原生栈进一步拆分。

快照：GPU总使用在编辑态约4161MiB，PIE约6521MiB；DWM专用显存约1675MiB，系统可用内存约4.6GB。DWM回升与复发同时存在，但额外1GB探针此前没有触发，因此仍只是相关性。

原始证据在`Saved/Profiling/Stutter-20261101-Recurrence/`：四份trace/统计、comparison.json、editor-observed.log，以及UI逐事件导出的ui-events.csv。此轮采用`TimingInsights.ExportTimingEvents ... -columns=*`拆分线程时间区间；导出中`Duration=inf`代表未闭合区间，不纳入有限耗时统计。

系统级采集尝试：确认WPR原先未录制后运行`wpr -start CPU -start GPU -filemode`，失败为`0xc5585011 Failed to enable the policy to profile system performance`，随后确认仍未录制，没有残留会话。REA `start_frida_session(mode=attach,pid=160380)`也失败：Frida原生绑定缺失（capability_unavailable），未建立会话，未注入脚本、未修改进程。未为此安装/修复工具或修改系统策略。

已向用户提供管理员PowerShell启动WPR的具体命令，等待系统级采集。管理员采集成功前，不声称已有Windows/驱动内部栈。Standards：保留新功能的源码/资产改动，未构建、测试、Cook或打包；Spec：较轻复发已捕获但根因仍未确定，性能风险未解除。

## 第一份Windows系统记录（02:02～02:04）

用户在管理员PowerShell启动CPU+GPU WPR并保存。非管理员上下文的`wpr -status`误导性地显示未录制，但`logman query -ets`实际存在`WPR_initiated_WprApp_WPR System Collector`和`WPR Event Collector`，TEMP下相应ETL也正在更新，因此通过系统会话及文件验证，而非重复启动。非管理员`wpr -stop`报0xc5583000，最终由用户在同一管理员终端停止保存。

系统记录`Saved/Profiling/Stutter-20261101-System/system.etl`共6,283,067,392字节，UTC18:02:25.2248955～18:04:27.6687252（本地02:02:25～02:04:27）。实际复现本地02:03:11.974～02:03:23.413，对应ETL起点后约46.749～58.188秒；配对UE trace为`system-paired.utrace`，含cpu/frame/gpu/bookmark/log/task，标记PIE开始与OpenInventory。

配对UE实测：最大CPU帧1587.822ms；CreateCommittedResource共107次，最大396.039ms；OpenInventory本身4.578ms；着色器等待0.021ms。保留`resource-events.csv`中的逐线程区间。**这些值是在系统采集启用时测得，不能直接当作未采集现场的性能。**

系统数据质量：xperf明确报告60,481,562个事件丢失，Lost Buffers=0。初次分析拒绝处理，随后显式使用`-tle`仅探索保留下来的事件，不将任何完整占比、缺失事件或完整等待链视作已验证。

已导出复现时段的模块采样`cpu-module.csv`、全部类型栈的`ue-stacks.txt`（实际是HTML）及结构化`stack-tables.json`。全部类型栈中存在大量EtwWriteTransfer、EtwpEventWriteFull、EtwpExpandStackAndWalkFrameChain及dxgkrnl事件写入路径；表内hits/percent是该报告的事件栈命中，**不是CPU时间百分比**，不能据此声称驱动或内核占用98%CPU。UE引擎PDB未安装，本机UE模块多处仍为unknown+偏移；微软公共符号已解析部分Windows函数。

尝试以`SampledProfile`和`SampleProfile`筛选普通CPU事件栈，未导出可用条目。这不证明程序没有运行，也不证明该段没有CPU开销；结合丢失事件和采集写栈路径，只将本份系统记录视作受采集干扰的有限证据，未确认原故障根因。采样事件与事件栈的区分参考[微软SampledProfile说明](https://learn.microsoft.com/en-us/windows/win32/etw/sampledprofile)。

已向用户请求CPU-only补采：管理员依次`wpr -start CPU -filemode`、等待45秒、`wpr -stop .../cpu-only.etl`，限定记录长度，不再启用GPU provider。当前第一份WPR会话已结束，PIE/UE trace停止、后台CPU节流恢复true。未改游戏源码/资源，未重启现场或执行构建、测试、Cook、打包。

## 零丢失CPU-only记录：确认资源驻留等待（02:17～02:19）

用户启动CPU-only WPR后，本轮复现开始于本地02:17:33.878，结束02:17:49.278。原拟45秒自动停止没有实际执行，检测到仍录制后请用户手动停止；最终`cpu-only.etl`为2,503,999,488字节，记录02:17:18.8282239～02:19:19.5229840，**Lost Events=0、Lost Buffers=0**，检查命令`xperf -i cpu-only.etl -o cpu-quality.txt -a tracestats`退出0。

配对`cpu-paired.utrace`最大帧2684.919ms；CreateCommittedResource最大289.633ms；OpenInventory本身4.062ms；着色器等待0.021ms。CPU-only仍包含采集开销，不能直接用该帧峰值推断未采集现场的严重程度。

使用Windows GetThreadDescription只读取得线程名：GameThread=159036、RenderThread 0=160780、RHIThread=160776。原始映射在`thread-names.json`。以Windows Performance Analyzer的CPU Usage (Precise)/Thread Delays表导出ETL第15～20秒，筛选这三个线程；保留`WaitFocused/CPU_Usage_(Precise)_Thread_Delays.csv`、`long-waits.json`和`residency-waits.json`。扩展栈会重复显示同一事件的多个层级，摘要按线程ID+Switch-In Time去重并取完整栈，不能累加这些重复行。

**直接阻塞位置已确认**：RenderThread 0的栈跨过UE D3D12RHI后进入：

```text
D3D12Core!CDevice::CreateCommittedResource
  → CDevice::CreateTestResourceAndHeap_Worker
  → CHeap::LUCCompleteLayerConstruction
  → CDevice::MakeResident_Worker
  → PerNodePagingOperation::Wait
  → win32u!ZwGdiDdDDIWaitForSynchronizationObjectFromCpu
  → dxgkrnl!DxgkWaitForSynchronizationObjectFromCpu
  → ntoskrnl!KeWaitForSingleObject
```

| RenderThread Switch-In Time（相对ETL起点，秒） | 驻留等待ms | Ready调度延迟ms |
| --- | ---: | ---: |
| 17.370448500 | 173.1145 | 0.0077 |
| 17.608790700 | 237.6709 | 0.0109 |
| 17.848769400 | 239.5114 | 0.0097 |
| 18.315456600 | 228.8374 | 0.0104 |
| 18.529584700 | 213.6572 | 0.0082 |
| 20.185534500（等待起点在所选窗口内） | 276.6826 | 0.0102 |

选中窗口内识别到83个带MakeResident/PagingOperation的渲染线程等待事件；其中Ready最大0.0292ms。说明这些已确认区间主要在等待资源驻留/分页完成，而非被唤醒后长时间抢不到CPU。**不据此排除其他帧或其他线程的调度问题，也不把所有UI等待都等同于这条栈。**

沟通过程中的一次数值更正：RenderThread在19.691003300秒还存在674.1801ms普通WaitForSingleObject等待（Ready0.0095ms）；完整栈不含MakeResident，因此不能把674ms记为已确认的驻留等待。驻留路径已明确的最大值采用276.6826ms，已向用户更正。

由此可解释“首次出现对象/界面时卡住”的一部分机制：新图形资源创建需要驻留，CPU在分页同步点等待，随后上层渲染/游戏线程等待任务完成。单靠任务管理器的全局显存总量不足以判定这里为何等待；此前额外1GB探针未触发也不能排除进程预算、驻留历史或分页队列因素。

仍未确认：等待变慢源自显存预算/驻留压力、Windows图形调度状态、驱动行为或其他因素；没有证明Live Coding、MCP或某次构建/测试是原始触发源。后续优先选择保持内容不变的图形路径/驻留状态对照，不继续盲目重复业务测试或打包。未修改默认RHI、驱动、DDC或系统策略。

CPU Usage (Sampled)导出在`CpuExport/`；其主要UE条目的采样栈为空，因此不能拿全部类型栈的事件命中率代替CPU时间。CPU Precise导出含所需等待栈，Windows公共符号足以确认上述系统/D3D12调用；UE引擎PDB缺失部分仍明确标注PDB not found，不猜具体UE私有函数。

本轮结束时WPR、PIE、UE trace均停止，后台节流恢复true，采样/导出进程结束，用户编辑器未重启，游戏源码/配置/地图/资产未改，既有其他功能改动保留。Standards：零丢失记录、线程身份、调用链和等待/Ready分别核对；Spec：直接阻塞机制取得新证据，但触发源及修复未验证，性能问题仍保持未解决。

## T06会话最后一轮工具审计

用户提供会话“接下来准备实施T06，先重新查看规格”，要求核对卡顿出现前的工具操作。通过Codex list_threads/read_thread与本地rollout交叉读取，threadId为`01a192f3-f78c-7fb1-a20b-d45c54af2856`，最后一轮北京时间2026-11-01 01:44:09～01:50:30，内容为修正金币/装备槽UI及临时发放测试装备。完整工具序列/参数留在`Saved/Profiling/Stutter-20261101-Recurrence/T06-tool-audit.md`，没有重放任何操作。

关键顺序：01:46:56调用list_dirty_packages、request_editor_shutdown(requireClean=true,endPIE=true)；01:47:10执行BuildEditor（真实UHT/UBT编译链接，-NoHotReloadFromIDE，01:47:32成功）；01:47:48独立命令行进程执行Wizard.Inventory七项测试，-NullRHI且禁用UE_MCP_Bridge；01:48:25调用start_editor，生成此前复发现场的PID160380；01:48:51起写入EquipmentSlotIcons、调整布局并编译InventoryMenu/InventoryTile；01:50:19这批调用最后打开InventoryMenu资产。其他工具主要为Widget/资产读回、未保存包检查及git diff检查。

同步核对子代理equipment_gas的本轮：仅修改WizardPlayerState.h/.cpp的TEMP T06发放代码并做静态检查；无UE MCP调用、Live Coding、构建或运行验证。主会话本轮也没有hot_reload、Agent启动PIE、渲染控制台配置、驱动变更、DDC清理、Cook或打包。blueprint.compile是Widget蓝图编译，不能算成C++ Live Coding；复发UE日志只有Live Coding初始化，没有补丁应用。

新增业务路径确有PIE首次初始化解析八件装备，以及背包刷新LoadSynchronous四个空槽图标；可以作为首次加载触发操作继续做定点测量，尚不能认定其造成资源驻留异常。现有OpenInventory约3.6ms与758ms帧停顿也不支持直接将整段耗时归为UI同步加载。冷构建/NullRHI测试未与该UE进程渲染同时进行；这削弱“本轮Live Coding或测试直接占GPU”的解释，但不能排除内存、系统状态、资产加载或累积因素。

**时间线边界**：当前卡顿调查首次用户报告是00:36:24；T06会话01:13:44才创建，晚于电脑00:56:55重启。因此该会话最后一轮无法触发00:36前的首次严重卡顿，可用于追查01:53后的较轻复发。此次只读审计未改变编辑器现场或游戏实现；触发源与修复仍未确认。

## 2026-11-02 Vulkan对照失败并崩溃

用户按建议以`-vulkan -sm6`启动，反馈卡顿未改善并崩溃。只读核对最新Saved/Logs/Wizard.log与Saved/Crashes/UECC-Windows-38B6582A401FFDB8D8F6F5A2C31FE452_0000/CrashContext.runtime-xml，确认实际RHI=Vulkan，GPU仍为RTX4060 Laptop、驱动581.42，没有误用DX12或集成显卡。本地03:16启动，03:26:43～46日志报告约6110～6123个Shader编译统计，03:27:20进入PIE，03:27:21起连续每0.5秒报告等待GPU遮挡查询结果超时；03:27:49触发VulkanCommandBuffer.cpp:503的check(bSuccess)，03:27:52记录RHIThread崩溃。该次日志仅有Live Coding启动初始化，无补丁应用。

本机UE5.4源码确认该断言位于FVulkanCommandBufferManager::WaitForCmdBuffer，bSuccess来自FenceManager.WaitForFence（GPU命令完成同步）；因此不是“首次新着色器编译导致编辑器暂时卡住”的解释。崩溃报告为Assert而非显式VK_ERROR_DEVICE_LOST，不能直接认定TDR或显卡硬件故障。查询03:15～03:35系统事件未发现Display/nvlddmkm/Dxg/WHEA提供程序匹配记录；仅限本次查询范围，不能当作驱动没有问题的证明。

本次结果否定“换Vulkan即可规避”在当前现场的效果，也削弱“问题仅限D3D12后端”的假设。此前D3D12驻留等待仍是有效观测，但Vulkan的GPU查询/Fence阻塞与它是否同一根因尚未证实；需同时保留共同系统/驱动/显存状态因素，以及独立Vulkan或渲染功能缺陷的可能，不能因两者都等待GPU就宣布同因。

配置读回DefaultGraphicsRHI仍为DX12，未发现Windows Vulkan Shader Formats的项目持久改动，Vulkan通过命令行强制使用。当前没有运行的UnrealEditor进程；本轮没有自行重启、PIE、构建、测试、清缓存或修改驱动/配置/游戏资源。下次正常打开项目不带-vulkan/-sm6即可回到项目默认DX12，但这只是撤回失败对照，不是卡顿修复。原始崩溃目录和minidump保留。其他较早的NullRHI自动化Ensure/Assert记录已区分，未混入本次Vulkan崩溃。

## 2026-11-02 驱动621.01仍卡顿；同进程空场景对照

用户升级驱动后仍卡顿。nvidia-smi与UE日志交叉确认RTX4060 Laptop驱动621.01（此前581.42），当前PID780292于03:44:19启动，实际默认D3D12，无ShaderCompileWorker/测试编辑器，Live Coding只有初始化。原地图03:45:37开始PIE，03:46:49～03:47:02出现3D与Compute队列5～12.5秒GPU timeout警告。升级没有解决当前症状，不能据此排除所有驱动/系统状态问题。

只读快照：电脑01:23:41启动，32GB系统可用约2.7GB，分页文件当前使用约4.8GB；Rider与Backend工作集合计约7.4GB，UE工作集约3.1GB/私有提交约5.6GB。未关闭其他应用。内存压力作为混杂条件记录，没有采到硬缺页与长停顿的直接对应，不认定为根因。HwSchMode/TdrDelay/TdrDdiDelay没有显式注册表值，不能仅凭缺省值断言实际HAGS状态。

用户明确允许约20秒空场景PIE对照并恢复地图。切换前内容/地图dirty均0、无PIE/trace。通过MCP加载引擎/Engine/Maps/Entry，仅在内存将其WorldSettings.DefaultGameMode设为/Script/Engine.GameModeBase，避免沿用项目默认Wizard GameMode而生成业务角色。基础模式由日志与get_world_settings读回确认。未保存引擎地图或用户地图。

第一次空场景记录empty.utrace，03:52:17启动、03:53:32请求停止，实际清理至03:53:40。原定短采样因开场长停顿和工具响应延迟延长，不能称为严格20秒窗口。CPU后台节流已关闭，但Slate限速自动开启，故不单凭帧耗时归因。完整统计CreateCommittedResource共93次、最大5.284047秒，Frame最大31.243089秒、AllocatePooledTextures最大25.243441秒；这些最大值覆盖空场景开场/运行/结束，不作为稳定运行平均帧率。首个抽样829ms，之后三次约16.67ms。

补采关闭Slate.bAllowThrottling与后台CPU节流；samples.json四次抽样为640.527ms、16.6668ms、16.6668ms、16.6668ms，warnings均空。empty-unthrottled.utrace覆盖第二次PIE及结束后的空场景编辑态，还记录到额外一次基础GameMode的PIE启动（日志03:57:11），因此不将整份统计视作单一20秒PIE。完整记录CreateCommittedResource140次、最大9.898819秒；该最大事件在UE trace相对时刻685.132335～695.031153，属于第二次PIE停止后的Entry编辑态。Frame最大64.048183秒，同样不能直接作为纯PIE帧峰值。逐事件resource-events.csv包含两次StartPlayInEditorSession与一次EndPlayMap供核对。

补采期间曾启动第一份记录的UnrealInsights导出进程，它虽带-NoUI仍初始化Standalone D3D，构成额外图形客户端混杂；已结束。不把第二份9.9秒与第一份5.3秒的差异作为性能变坏的因果证据。最终补采统计/事件使用-nullrhi导出，未初始化图形Renderer。保留两份trace、stats、samples.json、before-empty.log/after-empty.log与analysis日志在Saved/Profiling/Stutter-20261102-Driver621/；使用相对导出文件名产生的Engine/Binaries临时CSV已移至证据目录。没有新增系统WPR采集。

**本轮有效结论**：同一项目的空场景、基础GameMode下，也能捕获秒级D3D12资源创建停顿，Wizard背包/技能/掉落运行逻辑不是必要触发条件；场景仍运行在同一UE进程并保留项目插件、共享渲染设置和部分已加载资产，不能称为排除了整个项目或MCP。后续优先干净空项目、无Wizard/无MCP插件的同条件对照，区分项目/插件环境与共用UE/Windows/驱动状态，避免再把阻塞位置直接当根因。

stop_trace曾超时，先get_trace_status读回仍在记录，再以更长预算安全停止。恢复前确认唯一dirty包为本轮临时Entry，加载原/Game/Wizard/Maps/L_WizardClearing时仅丢弃该自有修改，返回discardedPackages仅/Engine/Maps/Entry。最终get_frame_timing确认tracing=false、后台CPU节流恢复true、Slate限速false（诊断前值）；dirty内容/地图均0，PIE已结束，原地图已恢复，分析进程结束。未构建、测试、Cook、打包或修改游戏源码/资源/配置，未更新驱动或关闭其他应用。Standards：控制GameMode/限速、记录采集干扰与时间范围、核对恢复结果；Spec：驱动升级及Vulkan均未提供修复，卡顿原因仍待隔离。

## 2026-11-02 用户补充：成品跨进程复现，其他UE5游戏正常

用户明确反馈并经两次追问确认：（1）关闭UE编辑器后，打包后的程序仍卡顿；（2）重启电脑后，只运行同一份打包程序、不打开UE/Rider/Codex，运行正常；（3）在Wizard已经卡顿的同一次开机期间，直接对照其他UE5游戏正常。成品卡顿/重启恢复以及其他游戏正常均为用户实测反馈，本轮未代替用户重跑成品，也未将其记为Agent已采样的成品CPU/GPU证据。其他游戏名称、UE具体版本、渲染后端和资源分配行为尚未取得，不能视作完全匹配的引擎对照。

只读核对Wizard.uproject的UE_MCP_Bridge.TargetAllowList只有Editor，桥接两个模块Type均Editor。找到本机候选成品Saved/Packages/20261102-025441-Package-33068c/Windows/Wizard/Binaries/Win64/Wizard.exe（293184512字节，02:55:39构建）；其现有日志02:57:40～02:59:02使用D3D12/581.42，无UE_MCP桥接加载。RenderDocPlugin虽被枚举，但明确打印不会加载捕获功能，不据此称已注入RenderDoc。此日志早于驱动621.01升级，不冒充新版驱动的成品现场，也尚未确认用户本次所用是否正是这个目录。

本轮只读快照：电脑仍01:23:41启动，驱动621.01，无运行中的UnrealEditor/Wizard进程；全局专用显存使用约859MiB/可用7098MiB，系统可用物理内存约6GB。该闲置快照不能代替卡顿时的进程预算/驻留记录，更不能排除进入特定渲染状态后的问题。系统事件与LiveKernelReports查询未获得可用于确诊的新报告。

额外发现：显示设备中OrayIddDriver Device、ToDesk Virtual Display Adapter均为Present/OK，另有AMD610M与RTX4060；SunloginService/ToDesk_Service为Stopped/Manual，未发现对应远控进程。设备存在不等于正在进行捕获，也不等于已定位根因；未停用、卸载或更改任何显示设备。间接显示驱动可参与图像处理和虚拟显示路径，背景参考[微软IDD说明](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/indirect-display-driver-model-overview)。WDDM按进程分配且可变的驻留预算参考[微软Process Residency Budgets](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/process-residency-budgets)，不能仅用全局显存剩余量宣布驻留压力已排除。

当前证据排序与可证伪预测：

1. 优先检查Wizard/UE5.4的渲染资源路径与可被重启清除的图形状态之间的交互。预测：故障现场的同版本干净最小程序若也出现相同资源创建长等待，则项目业务不是必要条件；若完全正常，则转查Wizard运行时渲染设置/插件/资源组合。不同版本UE5游戏正常不能代替此对照。
2. 保留进程预算、驻留历史以及混合显卡/显示路径等环境因素。预测：固定同一成品与操作，在异常和重启后状态采集对应等待/预算，可发现随状态变化的证据；未取得此证据前不直接改HAGS/MPO、TDR注册表或停用显示设备。
3. MCP/Live Coding在线执行或补丁累积不是成品复现的必要条件，但某次工具/构建操作是否曾触发跨进程状态，仍未排除。用户报告与Editor-only模块边界不足以回答原始触发源，不能倒推MCP完全无关。

后续对照固定同一份成品、同一启动参数与操作，不在两轮之间编译、Cook、打包、删缓存或改画质；将故障状态、UE5.4干净最小程序、重启后状态分别记录。不因本条反馈改成“所有UE5或所有图形应用都异常”，也不把重启有效当作驱动缺陷或项目代码错误的单独证明。本轮未启动程序/PIE、未重启电脑/编辑器、未更改配置、源码、资源或系统设置。Standards：用户实测与本地日志版本分开记录、排除在线编辑器必要性但保留触发源问题；Spec：根因与永久修复仍未验证。

## 2026-11-02 UE5.8项目也受影响：修正版本/项目边界

用户在故障期间打开之前从未出现此问题的UE5.8项目，同样卡顿；尚未做UE5.4独立空项目对照。此为重要新增反例，上一节“优先Wizard/UE5.4特定路径”的范围需要扩大，不能继续把问题限定于Wizard或UE5.4，更不能再把切换/升级引擎版本当作已有证据支持的修复。

只读找到D:/Game project(unreal)/Nexus/Saved/Logs/Nexus.log，确认Engine Version 5.8.0-55116800、默认D3D12、RTX4060（DeviceId28e0）、驱动621.01，启动参数-skipcompile；04:08起启动，04:16:47窗口关闭。日志Live Coding只有启动初始化，没有补丁记录。当前两个UE进程均已退出，电脑启动时间仍01:23:41，未发生新的重启。原始日志快照与逐项摘要保存在Saved/Profiling/Stutter-20261102-UE58/。

5.8日志明确出现7条GPU timeout：04:13:04的Graphics/Compute、04:14:25的Graphics/Compute、04:16:16的Graphics/Compute以及04:16:40的Compute，均超过5秒。04:13:04与04:14:25的Active GPU breadcrumb停留在SceneRender/ViewFamilies/AllocateRendertargets，下游VisibilityCommands/部分RDG工作尚未开始。该标记是执行进度快照，不能直接当作驱动栈、资源分配函数实际耗时或已证明的同一根因；但这已不是只有用户主观感知的另一项目卡顿，而有GPU队列等待证据，与5.4资源分配/驻留长等待构成相关线索。

Nexus配置也开启Lumen GI/反射、虚拟阴影，另开启硬件光追，不能视作与Wizard完全相同画质。其插件为5.8原生ModelContextProtocol/AllToolsets，日志存在初始化；不是Wizard的UE_MCP_Bridge。这不是完全无MCP的干净最小工程，也不能因为名称相近就认定是同一个插件或其造成了卡顿。本轮未通过任何MCP连接/操作Nexus，没有修改该项目文件或设置。

最新证据支持的工作划分：

- 受影响范围已跨项目、跨UE5.4/5.8，并包括无运行中编辑器的Wizard成品；编辑器在线活动和特定5.4进程内补丁不是必要条件。
- 其他UE5游戏在同次开机正常仍成立；这说明问题具有工作负载/路径选择性，不代表所有GPU任务都异常，也不能凭此否认两个项目日志里的GPU等待。
- “5.4/某次开发操作触发可持续状态，5.8后来只是受影响者”仍是待验证的时间线假设；没有受控触发实验，不能由打开顺序认定5.4、MCP、编译或测试是最初原因。
- 当前优先隔离跨应用共享图形状态，以及两项目共有的资源创建/驻留/显示路径；保留UE公共渲染实现与环境交互的可能。是否同一根因、哪一层首先异常仍未确认。

Standards：新增反例修正原假设、用5.8日志核对版本/设备/队列与breadcrumb解释边界；Spec：跨版本影响得到新证据，性能风险仍未关闭。本轮只读诊断及文档记录，未运行额外项目、PIE、构建、打包或修改驱动/系统/资源；未触发电脑重启。

## 2026-11-02 互联网同类案例检索

用户要求优先搜索他人是否遇到同样问题。本轮暂停新增复现实验，检索并实际打开Epic论坛原帖，按调用位置和症状匹配，不将普通低帧率/着色器首编当作同一问题。

最接近的公开记录为[Multi Second Freeze in FD3D12PoolAllocator::CreateNewPool / CreateCommittedResource](https://forums.unrealengine.com/t/multi-second-freeze-in-fd3d12poolallocator-createnewpool-createcommittedresource-after-upgrading-to-5-5-4/2670404)。2025-07起的报告同样包含秒级资源分配卡顿、Vulkan未改善；报告者提出重启之间反复启动后更易出现的假设。Epic支持人员审阅采样后指出案例中的Windows分配与UE临界路径锁竞争。该帖还有5.4/5.6报告，不能只按标题认作5.5专属。

对应[第二页](https://forums.unrealengine.com/t/multi-second-freeze-in-fd3d12poolallocator-createnewpool-createcommittedresource-after-upgrading-to-5-5-4/2670404?page=2)报告：扩大Upload池、延迟回收只缓解部分Nanite相关停顿，其他资源路径仍卡；另有人未用Nanite也复现。因此不能把池大小设置当作本机通用修复，尤其本机已有纹理/RenderTarget及MakeResident等待证据，而非只确认Upload池锁。

较新的Epic支持回复[How to setup asynchronous streaming and buffer pool to avoid frame drop，2026-03-30第18帖](https://forums.unrealengine.com/t/how-to-setup-asynchronous-streaming-and-buffer-pool-to-avoid-frame-drop/2724861/18)介绍了面向5.8的异步缓冲池预分配、延后到提交时驻留的改动，并提到正在与NVIDIA调查潜在驱动线程争用；没有宣称已证明每个案例都是该驱动问题。帖子具体列出CL50684724、51777483及相关后续修正，已公开改动信息不等于可直接回移到本机5.4或保证消除跨进程异常。

只读核对本机5.8源码D3D12Adapter.cpp确有D3D12.ResourcesStartResident，GD3D12StartResourceResident默认false、ECVF_ReadOnly；5.8 D3D12Allocation.cpp也存在UploadHeap/DefaultBuffer的OverFlowPoolsCount。由此确认相关机制存在于本机安装，而非照搬不存在的网络参数。仅核对源码，没有更改CVar、项目、引擎或实施补丁。

独立的症状相似报告[Editor Stuttering Bad on High End PC](https://forums.unrealengine.com/t/editor-stuttering-bad-on-high-end-pc/555218)描述多个UE版本/空项目偶发严重卡顿、重启电脑才暂时缓解，同时其他游戏正常；公开回复未给出经验证根因。其价值是相似现象已有他人报告，不能当作与本机同因的证明。

常见的[UE5.4极端卡顿/Studio Telemetry线程](https://forums.unrealengine.com/t/ue-5-4-and-5-4-1-editor-extreme-stutter-hitches/1843494)确有多人反馈关闭该插件有效，但原帖强调重启不改善、仅编辑器受影响，与本机成品复现及重启恢复存在关键差异，故不列为首要解释。[UE5.4.4/RTX4060 Laptop/32GB报告](https://forums.unrealengine.com/t/frequent-stuttering-and-frame-drops-in-editor/2486963)硬件相似，但没有原生等待栈或已验证解决方案，不因硬件匹配就认定同病。

检索结论：找到与阻塞函数及重启间复发描述高度相近的已公开问题族；外部资料支持优先研究UE资源/Heap分配临界路径与Windows/驱动的交互。本机已确认MakeResident/分页等待，但尚未实测证明是公开案例中的同一锁、同一缺陷或同一触发源，不能直接宣称“已查明NVIDIA驱动bug”。5.8改变分配/驻留时机，也不能仅凭升级后仍卡就排除资源驻留方向。原始触发与永久修复仍待证实。此次没有继续运行项目、构建、打包或修改系统。
