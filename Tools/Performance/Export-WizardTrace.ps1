#Requires -Version 7.0
<#
.SYNOPSIS
用本机 Unreal Insights 的 NoUI 批处理接口导出已经停止录制的 trace，不连接编辑器。
.EXAMPLE
pwsh -NoProfile -File Tools/Performance/Export-WizardTrace.ps1 -TracePath Saved/Profiling/Example.utrace -OutputDirectory Saved/T02/Performance/Trace-Example
#>
[CmdletBinding()]
param(
	[Parameter(Mandatory)] [string] $TracePath,
	[Parameter(Mandatory)] [string] $OutputDirectory,
	[string] $InsightsExecutable,
	[ValidatePattern('^[A-Za-z0-9_*,# ?.\-]+$')] [string] $EventThreads = 'GameThread,Render*,RHIThread'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

try {
	$ResolvedTrace = (Resolve-Path -LiteralPath $TracePath).Path
	if ([IO.Path]::GetExtension($ResolvedTrace) -ne '.utrace') { throw 'Expected a completed .utrace capture.' }
	if (!$InsightsExecutable) {
		$EngineRoot = Get-ItemPropertyValue -LiteralPath 'HKLM:/SOFTWARE/EpicGames/Unreal Engine/5.4' -Name InstalledDirectory
		$InsightsExecutable = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealInsights.exe'
	}
	$InsightsExecutable = (Resolve-Path -LiteralPath $InsightsExecutable).Path
	$Destination = [IO.Path]::GetFullPath($OutputDirectory)
	if (Test-Path -LiteralPath $Destination) { throw "Use a new output directory to avoid mistaking old exports for success: $Destination" }
	New-Item -ItemType Directory -Path $Destination | Out-Null
	$RspPath = Join-Path $Destination 'export.rsp'
	$LogPath = Join-Path $Destination 'insights.log'
	$ExportPaths = [ordered]@{
		threads = Join-Path $Destination 'threads.csv'
		timers = Join-Path $Destination 'timers.csv'
		events = Join-Path $Destination 'events.csv'
		statistics = Join-Path $Destination 'statistics.csv'
	}
	# 引擎 FParse::Token 支持带引号路径；使用正斜线避免反斜线被视为转义。
	$Commands = @(
		('TimingInsights.ExportThreads "{0}"' -f $ExportPaths.threads.Replace('\', '/'))
		('TimingInsights.ExportTimers "{0}"' -f $ExportPaths.timers.Replace('\', '/'))
		('TimingInsights.ExportTimingEvents "{0}" -columns=ThreadId,ThreadName,TimerId,TimerName,StartTime,EndTime,Duration,Depth -threads="{1}"' -f $ExportPaths.events.Replace('\', '/'), $EventThreads)
		('TimingInsights.ExportTimerStatistics "{0}"' -f $ExportPaths.statistics.Replace('\', '/'))
	)
	Set-Content -LiteralPath $RspPath -Value $Commands -Encoding utf8NoBOM
	$ProcessArguments = @(
		('-OpenTraceFile="{0}"' -f $ResolvedTrace), '-NoUI', '-Unattended', '-AutoQuit',
		('-ExecOnAnalysisCompleteCmd="@={0}"' -f $RspPath), ('-ABSLOG="{0}"' -f $LogPath)
	)
	# NoUI 为原生无人值守分析模式；不启动或操纵交互编辑器窗口。
	# UnrealInsights 为 Windows GUI 子系统程序；显式等待进程，不能依赖 LASTEXITCODE。
	$AnalysisProcess = Start-Process -FilePath $InsightsExecutable -ArgumentList $ProcessArguments -WindowStyle Hidden -PassThru
	$AnalysisProcess.WaitForExit()
	$AnalysisProcess.Refresh()
	$InsightsExitCode = $AnalysisProcess.ExitCode
	if ($InsightsExitCode -ne 0) { throw "Unreal Insights exited with $InsightsExitCode; inspect $LogPath" }
	$Files = @(
		foreach ($Kind in $ExportPaths.Keys) {
			if (!(Test-Path -LiteralPath $ExportPaths[$Kind])) { throw "Missing $Kind export; inspect $LogPath" }
			$Item = Get-Item -LiteralPath $ExportPaths[$Kind]
			if ($Item.Length -eq 0) { throw "Empty $Kind export; inspect $LogPath" }
			[pscustomobject]@{ kind = $Kind; path = $Item.FullName; bytes = $Item.Length; header = Get-Content -LiteralPath $Item.FullName -TotalCount 1 }
		}
	)
	$Summary = [pscustomobject]@{
		trace = $ResolvedTrace
		insightsExitCode = $InsightsExitCode
		noUI = $true
		log = $LogPath
		timingEventUnits = 'seconds'
		eventThreadFilter = $EventThreads
		exports = $Files
	}
	$Json = $Summary | ConvertTo-Json -Depth 5
	Set-Content -LiteralPath (Join-Path $Destination 'export-result.json') -Value $Json -Encoding utf8
	$Json
	exit 0
}
catch {
	Write-Error -Message $_ -ErrorAction Continue
	exit 2
}
