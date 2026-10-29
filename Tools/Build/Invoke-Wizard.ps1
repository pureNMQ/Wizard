#requires -Version 7.0
[CmdletBinding()]
param(
	[ValidateSet('BuildEditor', 'Test', 'Package', 'Smoke', 'All')]
	[string] $Action = 'BuildEditor',
	[string] $EngineRoot,
	[string] $PackageRoot,
	[ValidatePattern('^Wizard(?:\.[A-Za-z0-9_]+)*\.?$')]
	[string] $TestFilter = 'Wizard.',
	[switch] $CombatSmoke,
	[switch] $SkillSmoke,
	[switch] $SmokeNullRHI,
	[ValidateRange(2, 60)]
	[int] $SmokeSeconds = 10
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
$ProjectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$ProjectPath = Join-Path $ProjectRoot 'Wizard.uproject'
$ProjectDescription = Get-Content -LiteralPath $ProjectPath -Raw | ConvertFrom-Json

if (-not $EngineRoot) { $EngineRoot = $env:WIZARD_UE_ROOT }
if (-not $EngineRoot) {
	$InstallKey = "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$($ProjectDescription.EngineAssociation)"
	$EngineRoot = (Get-ItemProperty -LiteralPath $InstallKey -Name InstalledDirectory -ErrorAction Stop).InstalledDirectory
}
$EngineRoot = (Resolve-Path -LiteralPath $EngineRoot).Path
$BuildTool = Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat'
$AutomationTool = Join-Path $EngineRoot 'Engine/Build/BatchFiles/RunUAT.bat'
$EditorTool = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
foreach ($RequiredFile in @($BuildTool, $AutomationTool, $EditorTool)) {
	if (-not (Test-Path -LiteralPath $RequiredFile -PathType Leaf)) { throw "Missing engine tool: $RequiredFile" }
}

# 每轮采用独立输出目录，避免上次成品残留掩盖漏打资源；不清理用户文件。
$RunId = '{0}-{1}-{2}' -f (Get-Date -Format 'yyyyMMdd-HHmmss'), $Action, ([Guid]::NewGuid().ToString('N').Substring(0, 6))
$RunDirectory = Join-Path $ProjectRoot "Saved/T01/$RunId"
New-Item -ItemType Directory -Path $RunDirectory -Force | Out-Null
$Evidence = [ordered]@{
	action = $Action
	startedAt = (Get-Date -Format o)
	engineRoot = $EngineRoot
	engineVersion = (Get-Content (Join-Path $EngineRoot 'Engine/Build/Build.version') -Raw | ConvertFrom-Json)
	project = $ProjectPath
	checks = @()
	success = $false
}

function Save-Evidence {
	$Evidence | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $RunDirectory 'result.json') -Encoding utf8
}

function Assert-EditorClosed {
	$RunningEditor = Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe'" |
		Where-Object { $_.CommandLine -and $_.CommandLine.Contains($ProjectPath) }
	if ($RunningEditor) { throw 'Close and save the Wizard editor before compiling or running isolated tests.' }
}

function Invoke-NativeTool {
	param([string] $FilePath, [string[]] $NativeArguments, [string] $Name)
	$OutputLog = Join-Path $RunDirectory "$Name-console.log"
	Write-Host "[$Name] $FilePath"
	& $FilePath @NativeArguments 2>&1 | ForEach-Object { $_.ToString() } | Tee-Object -FilePath $OutputLog
	$NativeExit = $LASTEXITCODE
	$Evidence.checks += [ordered]@{ name = $Name; executable = $FilePath; arguments = $NativeArguments; exitCode = $NativeExit; log = $OutputLog }
	Save-Evidence
	if ($NativeExit -ne 0) { throw "$Name failed with exit code $NativeExit. See $OutputLog" }
}

function Build-Editor {
	Assert-EditorClosed
	Invoke-NativeTool -FilePath $BuildTool -Name 'build-editor' -NativeArguments @(
		'WizardEditor', 'Win64', 'Development', "-Project=$ProjectPath", '-WaitMutex', '-NoHotReloadFromIDE'
	)
}

function Test-Project {
	Assert-EditorClosed
	# 从测试源码收集预期名称，旧 DLL 漏掉新增测试时也必须失败。
	$ExpectedTests = @(Get-ChildItem -LiteralPath (Join-Path $ProjectRoot 'Source/Wizard/Private/Tests') -Filter '*.cpp' -File -Recurse |
		ForEach-Object {
			$SourceText = Get-Content -LiteralPath $_.FullName -Raw
			foreach ($Match in [regex]::Matches($SourceText, 'IMPLEMENT_SIMPLE_AUTOMATION_TEST\s*\(\s*\w+\s*,\s*"([^"\r\n]+)"')) {
				$TestName = $Match.Groups[1].Value
				if ($TestName.StartsWith($TestFilter, [StringComparison]::OrdinalIgnoreCase)) { $TestName }
			}
		} | Sort-Object -Unique)
	if ($ExpectedTests.Count -eq 0) { throw "No source tests match TestFilter '$TestFilter'." }
	$Evidence.testFilter = $TestFilter
	$Evidence.expectedTests = $ExpectedTests
	Save-Evidence
	$ReportDirectory = Join-Path $RunDirectory 'AutomationReport'
	$EngineLog = Join-Path $RunDirectory 'automation-engine.log'
	Invoke-NativeTool -FilePath $EditorTool -Name 'automation' -NativeArguments @(
		$ProjectPath, '-unattended', '-nop4', '-nosplash', '-nosound', '-NullRHI', '-UTF8Output',
		'-DisablePlugins=UE_MCP_Bridge',
		"-ExecCmds=Automation RunTests $TestFilter", '-TestExit=Automation Test Queue Empty',
		"-ReportExportPath=$ReportDirectory", "-abslog=$EngineLog"
	)
	$ReportPath = Join-Path $ReportDirectory 'index.json'
	if (-not (Test-Path -LiteralPath $ReportPath)) { throw "Automation report was not produced: $ReportPath" }
	$Report = Get-Content -LiteralPath $ReportPath -Raw | ConvertFrom-Json
	$ActualTests = @($Report.tests)
	foreach ($ExpectedTest in $ExpectedTests) {
		$Match = @($ActualTests | Where-Object { $_.fullTestPath -eq $ExpectedTest })
		if ($Match.Count -ne 1 -or $Match[0].state -ne 'Success') { throw "Expected test did not pass: $ExpectedTest" }
	}
	if ($ActualTests.Count -eq 0 -or @($ActualTests | Where-Object { $_.state -ne 'Success' }).Count -ne 0 -or $Report.failed -ne 0 -or $Report.notRun -ne 0) { throw 'Automation has failed or unexecuted tests.' }
	$Evidence.checks += [ordered]@{ name = 'automation-report'; passed = $Report.succeeded; failed = $Report.failed; notRun = $Report.notRun; report = $ReportPath }
	Save-Evidence
}

function Package-Project {
	Assert-EditorClosed
	if ($script:PackageRoot) { throw 'PackageRoot is an existing package input for Smoke. Package always creates a fresh output.' }
	$script:PackageRoot = Join-Path $ProjectRoot "Saved/Packages/$RunId"
	$UatLogRoot = Join-Path $RunDirectory 'UAT'
	$PreviousUatLogRoot = $env:uebp_LogFolder
	try {
		$env:uebp_LogFolder = $UatLogRoot
		Invoke-NativeTool -FilePath $AutomationTool -Name 'package' -NativeArguments @(
			'BuildCookRun', '-unattended', '-UTF8Output', "-project=$ProjectPath", '-noP4', '-installed',
			'-platform=Win64', '-clientconfig=Development', '-build', '-cook',
			'-map=/Game/Wizard/Maps/L_WizardClearing', '-stage', '-pak', '-package', '-archive',
			"-archivedirectory=$script:PackageRoot", '-prereqs'
		)
	}
	finally { $env:uebp_LogFolder = $PreviousUatLogRoot }
	$Evidence.packageRoot = $script:PackageRoot
	Save-Evidence
}

function Test-PackagedGame {
	if ($CombatSmoke -and $SkillSmoke) { throw 'Choose one of -CombatSmoke and -SkillSmoke.' }
	if (-not $script:PackageRoot) { throw 'Smoke requires -PackageRoot from a successful Package run.' }
	$ResolvedPackage = (Resolve-Path -LiteralPath $script:PackageRoot).Path
	$GameExecutables = @(Get-ChildItem -LiteralPath $ResolvedPackage -Recurse -File -Filter 'Wizard.exe' |
		Where-Object { $_.FullName -match '[\\/]Wizard[\\/]Binaries[\\/]Win64[\\/]' })
	if ($GameExecutables.Count -ne 1) { throw 'Expected one real Wizard/Binaries/Win64/Wizard.exe in the package.' }
	$GameFile = $GameExecutables[0].FullName
	$SmokeName = if ($SkillSmoke) { 'packaged-skills-smoke' } elseif ($CombatSmoke) { 'packaged-combat-smoke' } else { 'packaged-smoke' }
	$SmokeLog = Join-Path $RunDirectory "$SmokeName.log"
	$SmokeCommand = if ($SkillSmoke) { 'Wizard.SkillsSmokeTest' } elseif ($CombatSmoke) { 'Wizard.CombatSmokeTest' } else { 'Wizard.SmokeTest' }
	$PassMarker = if ($SkillSmoke) { 'WIZARD_SKILLS_SMOKE_PASS:' } elseif ($CombatSmoke) { 'WIZARD_COMBAT_SMOKE_PASS:' } else { 'WIZARD_SMOKE_PASS:' }
	# 自动检查隐藏启动以免抢占用户输入；检查完成后由游戏正常退出。
	$SmokeArguments = @('-windowed', '-ResX=1280', '-ResY=720', '-nosplash', '-unattended', '-UTF8Output',
		('-ExecCmds="{0},HighResShot 1280x720"' -f $SmokeCommand), "-WizardSmokeSeconds=$SmokeSeconds", ('-abslog="{0}"' -f $SmokeLog))
	if ($SmokeNullRHI) { $SmokeArguments += '-nullrhi' }
	$SmokeProcess = Start-Process -FilePath $GameFile -WorkingDirectory (Split-Path $GameFile) -ArgumentList $SmokeArguments -WindowStyle Hidden -PassThru
	$Finished = $SmokeProcess.WaitForExit(180000)
	if (-not $Finished) {
		# 只结束由本次验证启动且持有句柄的进程，不按名称结束用户的其他游戏。
		$SmokeProcess.Kill()
		$SmokeProcess.WaitForExit()
		throw "Packaged smoke timed out. See $SmokeLog"
	}
	$SmokeExit = $SmokeProcess.ExitCode
	$SmokeText = if (Test-Path -LiteralPath $SmokeLog) { Get-Content -LiteralPath $SmokeLog -Raw } else { '' }
	if ($SmokeExit -ne 0 -or $SmokeText -notmatch $PassMarker) { throw "Packaged startup/movement failed (exit $SmokeExit). See $SmokeLog" }
	if ($SmokeText -match 'Mounting project plugin UE_MCP_Bridge|LogMCPBridge:') { throw 'Editor bridge unexpectedly loaded in the packaged game.' }
	$Evidence.checks += [ordered]@{ name = $SmokeName; exitCode = $SmokeExit; executable = $GameFile; log = $SmokeLog; bridgeLoaded = $false; renderingEnabled = (-not $SmokeNullRHI) }
	Save-Evidence
}

try {
	switch ($Action) {
		'BuildEditor' { Build-Editor }
		'Test' { Test-Project }
		'Package' { Package-Project }
		'Smoke' { Test-PackagedGame }
		'All' { Build-Editor; Test-Project; Package-Project; Test-PackagedGame }
	}
	$Evidence.success = $true
	$Evidence.finishedAt = Get-Date -Format o
	Save-Evidence
	Write-Host "PASS: $Action. Evidence: $RunDirectory"
	if ($script:PackageRoot) { Write-Host "Package: $script:PackageRoot" }
}
catch {
	$Evidence.error = $_.Exception.Message
	Save-Evidence
	Write-Error $_
	exit 1
}
