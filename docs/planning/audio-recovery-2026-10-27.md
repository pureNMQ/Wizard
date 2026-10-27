# 音效恢复验证

日期：2026-10-27。

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

## 仍未完成的内容

- 主项目尚未导入恢复后的 WAV，因此旧音效目录目前仍不能被 UE5.4 直接使用。
- 其余 203 条 WAV 尚未逐条完成 UE5.4 导入/重新导出；其结构、时长与声道检查已经完成。
- 105 个 SoundCue 的节点图尚未转换。需要保留随机、混音、调制、衰减或其他节点行为时，必须另行读取并在 UE5.4 重建相应配置；不能把 WAV 恢复称为整个 SoundCue 包完全恢复。
- 尚未进行全部音效试听、实际游戏播放或 Cook/打包验证。

对于当前 Demo，可直接选用恢复的 SoundWave，按项目需要建立音量、衰减和 SoundCue 配置。后续导入建议使用新的项目目录并保留原素材，确认运行结果后再处理旧目录。

## 参考

- [Epic 资产管理与导出说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/working-with-assets-in-unreal-engine)。
- [Epic Python 编辑器脚本与命令行](https://dev.epicgames.com/documentation/en-us/unreal-engine/scripting-the-unreal-editor-using-python?application_version=5.8)。
- 导出接口以本机 `Engine/Source/Editor/UnrealEd/Classes/Exporters/SoundExporterWAV.h` 和实际执行结果为依据。
