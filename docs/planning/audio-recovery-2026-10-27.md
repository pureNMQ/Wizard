# 音效恢复验证

日期：2026-10-27。

## 当前状态：已补齐主项目导入并移除不兼容原包

前一阶段只完成 WAV 导出和独立项目验证，没有导入主项目，因此用户在 UE5.4 中仍看不到原包音效。现已补齐导入，以下状态覆盖本文后面的历史阶段记录。

- 205 个 WAV 源文件保存在 `SourceAssets/Audio/Recovered`，按原素材包和类别保留目录层级；WAV 使用 Git LFS。
- 205 个 SoundWave 全部导入并保存到 `/Game/Audio`。内容浏览器的实际路径为 `内容 / Audio`，其下包括 `RPG_Fantasy_SFX` 和 `UltimateUIMenusSFX`；不要继续在原不兼容目录查找恢复资产。
- 当前编辑器资产注册表列出 205 个恢复 SoundWave；磁盘上的 UE5.4 `.uasset` 也为 205 个。
- 在独立 UE5.4.4 commandlet 进程中重新加载这些已保存资产并导出 WAV，205/205 通过，失败 0。全部 PCM SHA-256、声道、采样率、采样宽度和帧数与源 WAV 相同；时长、音量、音高及循环属性也一致。
- 已实际打开恢复的法术音效 `RPG_Fantasy_SFX/Spell/8__1_`，界面中可见 SoundWave 属性和内容浏览器中的波形缩略图。没有逐条进行人工试听，也尚未验证这些音效在游戏逻辑中的使用。
- 310 个不兼容原包已从项目移出，原文件完整保留在工程外备份中；105 个 SoundCue 节点图仍未恢复。清理后的 Windows 迭代 Cook 为 0 错误、0 告警。

来源与目标映射、源 WAV 文件摘要见 `SourceAssets/Audio/Recovered/manifest.json`。可重复执行以下校验，生成日志和重新导出的 WAV 位于 Git 忽略的 `Saved/AudioImportChecks/20261027/`，`validation-result.json` 记录逐条结果：

```powershell
$EngineRoot = 'D:\Program Files\Epic Games\UE_5.4'
$ProjectFile = (Resolve-Path '.\Wizard.uproject').Path
$ValidationScript = (Resolve-Path '.\Tools\Audio\validate_recovered_audio.py').Path
& "$EngineRoot\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $ProjectFile -run=pythonscript "-script=$ValidationScript" -unattended -nop4 -NullRHI -DisablePlugins=UE_MCP_Bridge
```

## 不兼容原目录清理结果

用户确认移除原不兼容目录后，检查了 310 个原包的已保存反向引用，目录外引用为 0；代码与配置中也未发现原资源路径引用。编辑器没有未保存资产或地图，正常关闭后再处理磁盘文件。

自动审批拒绝了递归删除命令，只返回 `blocked by policy`。实际使用可恢复的移动操作，将以下两个目录从主项目 `Content` 移到 `D:\Game project(unreal)\Wizard_AssetBackups\20261027_incompatible_audio`，没有执行永久删除：

| 原目录 | 移出包数 |
| --- | --- |
| `Content/RPG_Fantasy_SFX` | 100 |
| `Content/UltimateUIMenusSFX` | 210 |

共移出 310 个 `.uasset`，48,407,378 字节。移动前验证了既有完整 ZIP 备份中的对应文件；移动后再次验证工程外文件哈希，310/310 与清理前一致。205 个恢复 SoundWave 的清理前后哈希全部相同，WAV 源文件保留在项目 `SourceAssets` 中。

重新启动 UE5.4 后，资产注册表仍列出 205 个恢复 SoundWave。对 `/Game/TopDown/Maps/TopDownMap` 执行 Windows 迭代 Cook，进程返回 0，摘要为 `Success - 0 error(s), 0 warning(s)`。这是当前关卡的迭代 Cook 验证，不代表成品打包或全部音效的游戏内试听。

清理清单、文件摘要、验证结果及 Cook 日志位于 Git 忽略的 `Saved/AudioCleanupChecks/20261027/`。旧原包也保留在已推送的 `baseline-2026-10-27` 标签中；本次没有移动该标签。

## 后续目录整理与冗余资源剔除

用户已将恢复后的资产目录调整为 `Content/Audio/RPG_Fantasy_SFX` 和 `Content/Audio/UltimateUIMenusSFX`，去除内容目录中的 `Recovered` 层级。WAV 源文件继续位于 `SourceAssets/Audio/Recovered`。已将来源清单中的 205 个目标路径全部同步到新目录，验证脚本同时检查 SoundWave 的重新导入路径是否仍指向对应源 WAV。

用户删除的冗余展示素材为 `Content/Fab/Mini_Simple_Characters___Skeleton__Free_Demo_`，共 29 个包。提交前对全部 1,959 个剩余注册资产读取硬引用和软引用，未发现指向这 29 个包、此前移出的 310 个不兼容原包，或旧 `Audio/Recovered` 目标路径的保存依赖。

目录整理后，在 UE5.4.4 中重新加载并导出全部 205 条音效，205/205 通过，PCM 数据、属性及重新导入源路径全部正确；Windows 迭代 Cook 返回 0，摘要为 0 错误、0 告警。此次校验日志位于 `Saved/PreCommitChecks/20261027-audio-layout/`，详细音效结果仍由 `Saved/AudioImportChecks/20261027/validation-result.json` 输出。README.md 保持用户要求的 0 字节空白。

## 第一阶段记录：导出与样本验证

## 结论与实际范围

可以通过“较新引擎读取 → 导出 WAV → UE5.4 导入”恢复这些包中的音频。已完成全部音频导出、WAV 结构校验和两条代表音效的 UE5.4 往返验证。

本次没有替换主项目资产，也没有修改原包头。主项目仍为 UE5.4；UE5.8 仅用于临时转换项目。

## 验证结果

| 检查 | 结果 |
| --- | --- |
| 原资产分类 | 205 个 SoundWave、105 个 SoundCue，共 310 个包 |
| UE5.8 导出 | 205 个 SoundWave 全部导出成功，失败 0 |
| WAV 检查 | 205 个均为有效 16 位 PCM WAV；帧数有效，时长和声道与源资产属性匹配 |
| UE5.4 导入样本 | RPG Spell/8__1_、UI CasualMobile_01，均成功导入及保存 |
| UE5.4 重新导出 | 两条样本的 PCM 数据和音频参数与导入前一致，PCM SHA-256 相同 |
| 原文件保护 | 310 个原资产文件的转换前后 SHA-256 全部相同 |
| 导出音频体积 | WAV 合计 111,017,008 字节 |
| 恢复包 | ZIP 含 205 个 WAV、中文说明、源属性映射和验证 JSON；压缩包完整性检查通过 |

实际使用引擎为 UE5.8.0 和 UE5.4.4。没有将主项目升级到 UE5.8。

## 文件位置

- [恢复 WAV 压缩包](<C:/Users/pureNMQ/.codex/visualizations/2026/10/27/01a17c76-ae37-7ed1-a415-e3a263e6592f/AudioRecovery/Wizard_Recovered_WAV_20261027.zip>)。
- 临时转换工程和脚本：`C:\Users\pureNMQ\AppData\Local\Temp\WizardAudioRecovery_m5cxnlrd`。
- 压缩包保留两个素材包的目录层级；`recovery-metadata.json` 记录原资产路径、对应 WAV、音量/音高/循环等属性及文件摘要。
- `validation.json` 记录 WAV 检查、两条样本 PCM 比较及原文件保护结果。

## 操作机制

1. 将两套素材复制到独立的临时 UE5.8 内容项目，原项目保持原样。
2. 使用 UnrealEditor-Cmd 的 Python commandlet 加载 SoundWave，通过引擎的 SoundExporterWAV 导出。
3. 对全部 WAV 检查格式、时长、声道和数据长度。
4. 在另一个临时 UE5.4 项目中使用 AssetImportTask 导入两条代表音效，保存为该版本资产后重新导出。
5. 比较 WAV 的实际 PCM 内容，不依赖整个 WAV 文件二进制一致，因为容器中的非音频块可以不同。

这些 Python 脚本仅处理离线素材，不属于游戏运行时逻辑。

## 第一阶段结束时未完成的内容（历史记录）

- 主项目尚未导入恢复后的 WAV，因此旧音效目录目前仍不能被 UE5.4 直接使用。
- 其余 203 条 WAV 尚未逐条完成 UE5.4 导入/重新导出；其结构、时长与声道检查已经完成。
- 105 个 SoundCue 的节点图尚未转换。需要保留随机、混音、调制、衰减或其他节点行为时，必须另行读取并在 UE5.4 重建相应配置；不能把 WAV 恢复称为整个 SoundCue 包完全恢复。
- 尚未进行全部音效试听、实际游戏播放或 Cook/打包验证。

对于当前 Demo，可直接选用恢复的 SoundWave，按项目需要建立音量、衰减和 SoundCue 配置。后续导入建议使用新的项目目录并保留原素材，确认运行结果后再处理旧目录。

## 参考

- [Epic 资产管理与导出说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/working-with-assets-in-unreal-engine)。
- [Epic Python 编辑器脚本与命令行](https://dev.epicgames.com/documentation/en-us/unreal-engine/scripting-the-unreal-editor-using-python?application_version=5.8)。
- 导出接口以本机 `Engine/Source/Editor/UnrealEd/Classes/Exporters/SoundExporterWAV.h` 和实际执行结果为依据。
