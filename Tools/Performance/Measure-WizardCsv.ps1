#Requires -Version 7.0
<#
.SYNOPSIS
读取 UE 原生 CSV Profiler 输出，以真实帧时间判定长停顿；不连接或修改编辑器。
.EXAMPLE
pwsh -NoProfile -File Tools/Performance/Measure-WizardCsv.ps1 -Path Saved/Profiling/CSV/WizardIdle.csv
#>
[CmdletBinding()]
param(
	[Parameter(Mandatory, Position = 0)] [string[]] $Path,
	[ValidateRange(0, 2147483647)] [int] $MaxFramesOver100Ms = 0,
	[ValidateRange(0, 2147483647)] [int] $MaxFramesOver500Ms = 0,
	[ValidateRange(0, 3600)] [double] $MinimumDurationSeconds = 8,
	[string] $OutputPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName Microsoft.VisualBasic.Core

function Read-FrameColumn([string[]] $Fields, [int] $Index) {
	if ($Index -lt 0 -or $Index -ge $Fields.Length) { return $null }
	[double] $Number = 0
	if ([double]::TryParse($Fields[$Index], [Globalization.NumberStyles]::Float,
		[Globalization.CultureInfo]::InvariantCulture, [ref] $Number) -and [double]::IsFinite($Number) -and $Number -ge 0) {
		return $Number
	}
	return $null
}

function Get-CsvFrameSummary([string] $CsvPath) {
	$Resolved = (Resolve-Path -LiteralPath $CsvPath).Path
	if ([IO.Path]::GetExtension($Resolved) -ne '.csv') { throw "Expected an uncompressed .csv: $Resolved" }
	# UE 的尾部 metadata（尤其 Commandline）不是标准 CSV 引号转义，不能送进数据解析器。
	# 先界定原始物理行边界，保留全部真实数据行；绝不捕获解析错误后静默跳过。
	$Lines = [IO.File]::ReadAllLines($Resolved, [Text.Encoding]::UTF8)
	$HeaderLines = @()
	for ($LineIndex = 0; $LineIndex -lt $Lines.Length; ++$LineIndex) {
		if ($Lines[$LineIndex].StartsWith('EVENTS,') -and $Lines[$LineIndex] -match '(?:^|,)FrameTime(?:,|$)') { $HeaderLines += $LineIndex }
	}
	if ($HeaderLines.Count -lt 2) { throw "CSV capture has no final summary header; wait for CsvProfile STOP to finish writing: $Resolved" }
	$FinalHeaderIndex = $HeaderLines[-1]
	$MetadataStartIndex = $FinalHeaderIndex + 1
	if ($MetadataStartIndex -ge $Lines.Length -or !$Lines[$MetadataStartIndex].StartsWith('[HasHeaderRowAtEnd],1')) {
		throw "Expected UE final-header metadata at physical line $($MetadataStartIndex + 1): $Resolved"
	}
	for ($LineIndex = $MetadataStartIndex + 1; $LineIndex -lt $Lines.Length; ++$LineIndex) {
		if (![string]::IsNullOrWhiteSpace($Lines[$LineIndex])) { throw "Unexpected content after UE metadata at physical line $($LineIndex + 1): $Resolved" }
	}
	$DataText = [string]::Join("`n", $Lines[0..$FinalHeaderIndex])
	$Parser = [Microsoft.VisualBasic.FileIO.TextFieldParser]::new([IO.StringReader]::new($DataText))
	$Parser.SetDelimiters(',')
	$Parser.HasFieldsEnclosedInQuotes = $true
	$Parser.TrimWhiteSpace = $false
	$Rows = [Collections.Generic.List[object]]::new()
	[string[]] $Header = @()
	$HasFinalHeader = $false
	try {
		while (!$Parser.EndOfData) {
			$SourceLine = $Parser.LineNumber
			$Fields = $Parser.ReadFields()
			if (!$Fields -or $Fields.Length -eq 0) { continue }
			if ($Fields[0] -eq 'EVENTS') {
				# UE5.4 可在采样中追加列，最终 EVENTS 表头才包含完整列集合。
				$HasFinalHeader = $Header.Length -gt 0
				$Header = $Fields
				continue
			}
			$Rows.Add([pscustomobject]@{ Fields = $Fields; SourceLine = $SourceLine })
		}
	}
	finally { $Parser.Dispose() }
	if (!$HasFinalHeader) { throw "CSV capture has no final summary header; wait for CsvProfile STOP to finish writing: $Resolved" }
	$FrameColumn = [Array]::IndexOf($Header, 'FrameTime')
	if ($FrameColumn -lt 0) { throw "Native FrameTime column missing: $Resolved" }
	$ColumnIndices = [ordered]@{
		GameThreadTime = [Array]::IndexOf($Header, 'GameThreadTime')
		RenderThreadTime = [Array]::IndexOf($Header, 'RenderThreadTime')
		GPUTime = [Array]::IndexOf($Header, 'GPUTime')
	}
	$Frames = [Collections.Generic.List[double]]::new()
	$Hitches = [Collections.Generic.List[object]]::new()
	[double] $CumulativeMs = 0
	foreach ($Row in $Rows) {
		$Fields = $Row.Fields
		$FrameMs = Read-FrameColumn $Fields $FrameColumn
		if ($null -eq $FrameMs) { throw "Invalid FrameTime at physical source line $($Row.SourceLine), data row $($Frames.Count): $Resolved" }
		if ($FrameMs -gt 100) {
			$Hitch = [ordered]@{ frameIndex = $Frames.Count; sourceLine = $Row.SourceLine; startSeconds = [Math]::Round($CumulativeMs / 1000, 4); frameMs = $FrameMs }
			foreach ($Column in $ColumnIndices.Keys) { $Hitch[$Column] = Read-FrameColumn $Fields $ColumnIndices[$Column] }
			$Hitches.Add([pscustomobject] $Hitch)
		}
		$Frames.Add($FrameMs)
		$CumulativeMs += $FrameMs
	}
	if ($Frames.Count -lt 2) { throw "Too few measured frames: $Resolved" }
	$Sorted = @($Frames | Sort-Object)
	$Over100 = @($Frames | Where-Object { $_ -gt 100 }).Count
	$Over500 = @($Frames | Where-Object { $_ -gt 500 }).Count
	$Seconds = $CumulativeMs / 1000
	$Reasons = [Collections.Generic.List[string]]::new()
	if ($Seconds -lt $MinimumDurationSeconds) { $Reasons.Add("Measured duration $Seconds < required $MinimumDurationSeconds seconds") }
	if ($Over100 -gt $MaxFramesOver100Ms) { $Reasons.Add("Frames >100ms: $Over100 > allowed $MaxFramesOver100Ms") }
	if ($Over500 -gt $MaxFramesOver500Ms) { $Reasons.Add("Frames >500ms: $Over500 > allowed $MaxFramesOver500Ms") }
	[pscustomobject][ordered]@{
		file = $Resolved
		finalHeaderSourceLine = $FinalHeaderIndex + 1
		metadataSourceLine = $MetadataStartIndex + 1
		frameCount = $Frames.Count
		measuredSeconds = [Math]::Round($Seconds, 4)
		maximumFrameMs = $Sorted[-1]
		p95FrameMs = $Sorted[[Math]::Max(0, [Math]::Ceiling($Sorted.Count * 0.95) - 1)]
		averageFrameMs = [Math]::Round($CumulativeMs / $Frames.Count, 4)
		framesOver100Ms = $Over100
		framesOver500Ms = $Over500
		passed = $Reasons.Count -eq 0
		failureReasons = @($Reasons.ToArray())
		hitches = @($Hitches.ToArray())
	}
}

try {
	$Summaries = @($Path | ForEach-Object { Get-CsvFrameSummary $_ })
	$Json = ConvertTo-Json -InputObject $Summaries -Depth 8
	if ($OutputPath) { Set-Content -LiteralPath $OutputPath -Value $Json -Encoding utf8 }
	$Json
	if (@($Summaries | Where-Object { !$_.passed }).Count -gt 0) { exit 1 }
	exit 0
}
catch {
	Write-Error -Message $_ -ErrorAction Continue
	exit 2
}
