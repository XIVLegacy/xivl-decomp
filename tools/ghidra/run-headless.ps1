<#
.SYNOPSIS
    Runs one Ghidra post-script in an isolated evidence directory.

.DESCRIPTION
    Binary, script, and output directory are mandatory. Relative paths resolve
    from the repository root. Output must be a new or empty directory below
    tools\ghidra\logs so raw logs, projects, and decompiled bodies stay ignored.

    Ghidra 12.1 and JDK 21 can be passed explicitly, supplied through
    GHIDRA_HOME and JAVA_HOME, or discovered in common per-user tool folders.

.EXAMPLE
    tools\ghidra\run-headless.ps1 `
        -Binary orig\ffxivgame.exe `
        -Script tools\ghidra_scripts\FindBytes.java `
        -OutputDirectory tools\ghidra\logs\find-mz `
        -ScriptEnvironment @{ SEARCH_BYTES = '4d 5a' }
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Binary,
    [Parameter(Mandatory = $true)][string]$Script,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [hashtable]$ScriptEnvironment = @{},
    [string[]]$ScriptArgument = @(),
    [string]$GhidraHome = $env:GHIDRA_HOME,
    [string]$JavaHome = $env:JAVA_HOME,
    [string]$MaxMemory = '8G',
    [int]$AnalysisTimeoutSeconds = 0,
    [switch]$AllowProgramWrites
)

$ErrorActionPreference = 'Stop'
$RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$LogsRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'logs'))

function Resolve-RepositoryPath {
    param([string]$Path)
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }
    return [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $Path))
}

function Convert-ToShortPath {
    param([string]$Path)
    if (-not ('XivlNativePath' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public static class XivlNativePath {
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern uint GetShortPathName(
        string longPath, StringBuilder shortPath, int bufferLength);
}
'@
    }
    $buffer = New-Object System.Text.StringBuilder 32768
    $length = [XivlNativePath]::GetShortPathName($Path, $buffer, $buffer.Capacity)
    if ($length -eq 0 -or $length -ge $buffer.Capacity) { return $Path }
    return $buffer.ToString()
}

function Assert-NoReparsePoint {
    param([string]$Path, [string]$StopAt)
    $current = [System.IO.Path]::GetFullPath($Path)
    $stop = [System.IO.Path]::GetFullPath($StopAt)
    while ($true) {
        if (Test-Path -LiteralPath $current) {
            $item = Get-Item -LiteralPath $current -Force
            if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Evidence output path crosses a reparse point: $current"
            }
        }
        if ($current.Equals($stop, [System.StringComparison]::OrdinalIgnoreCase)) {
            return
        }
        $parent = [System.IO.Directory]::GetParent($current)
        if ($null -eq $parent) {
            throw "Evidence output path does not descend from $stop"
        }
        $current = $parent.FullName
    }
}

function Get-GhidraVersion {
    param([string]$InstallRoot)
    $properties = Join-Path $InstallRoot 'Ghidra\application.properties'
    $headless = Join-Path $InstallRoot 'support\analyzeHeadless.bat'
    if (-not (Test-Path -LiteralPath $headless -PathType Leaf) -or
            -not (Test-Path -LiteralPath $properties -PathType Leaf)) {
        return $null
    }
    $line = Get-Content -LiteralPath $properties |
        Where-Object { $_ -match '^application\.version=' } |
        Select-Object -First 1
    if (-not $line) { return $null }
    return ($line -split '=', 2)[1].Trim()
}

function Resolve-GhidraInstall {
    param([string]$Requested)
    $candidates = New-Object System.Collections.Generic.List[string]
    if (-not [string]::IsNullOrWhiteSpace($Requested)) {
        $candidates.Add($Requested)
    }
    foreach ($root in @(
            (Join-Path $env:USERPROFILE 'Tools'),
            (Join-Path $env:USERPROFILE 'tools'))) {
        if (Test-Path -LiteralPath $root -PathType Container) {
            Get-ChildItem -LiteralPath $root -Directory -Filter 'ghidra_*_PUBLIC' |
                Sort-Object Name -Descending |
                ForEach-Object { $candidates.Add($_.FullName) }
        }
    }
    foreach ($candidate in ($candidates | Select-Object -Unique)) {
        $resolvedHome = [System.IO.Path]::GetFullPath($candidate)
        $version = Get-GhidraVersion $resolvedHome
        if ($version -eq '12.1') {
            return [pscustomobject]@{ Home = $resolvedHome; Version = $version }
        }
    }
    throw 'Ghidra 12.1 not found. Pass -GhidraHome or set GHIDRA_HOME to an installation containing support\analyzeHeadless.bat.'
}

function Get-JavaVersionText {
    param([string]$InstallRoot)
    $java = Join-Path $InstallRoot 'bin\java.exe'
    if (-not (Test-Path -LiteralPath $java -PathType Leaf)) { return $null }
    $start = New-Object System.Diagnostics.ProcessStartInfo
    $start.FileName = $java
    $start.Arguments = '-version'
    $start.UseShellExecute = $false
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $process = [System.Diagnostics.Process]::Start($start)
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) { return $null }
    return ($stderr + $stdout).Trim()
}

function Resolve-Jdk21 {
    param([string]$Requested)
    $candidates = New-Object System.Collections.Generic.List[string]
    if (-not [string]::IsNullOrWhiteSpace($Requested)) {
        $candidates.Add($Requested)
    }
    foreach ($spec in @(
            @{ Root = $env:ProgramFiles; Pattern = 'Eclipse Adoptium\jdk-21*' },
            @{ Root = $env:ProgramFiles; Pattern = 'Java\jdk-21*' },
            @{ Root = (Join-Path $env:USERPROFILE 'Tools'); Pattern = 'jdk-21*' },
            @{ Root = (Join-Path $env:USERPROFILE 'tools'); Pattern = 'jdk-21*' })) {
        if ($spec.Root -and (Test-Path -LiteralPath $spec.Root -PathType Container)) {
            Get-ChildItem -Path (Join-Path $spec.Root $spec.Pattern) -Directory -ErrorAction SilentlyContinue |
                Sort-Object Name -Descending |
                ForEach-Object { $candidates.Add($_.FullName) }
        }
    }
    $pathJava = Get-Command java.exe -ErrorAction SilentlyContinue
    if ($pathJava) {
        $candidates.Add((Split-Path (Split-Path $pathJava.Source -Parent) -Parent))
    }
    foreach ($candidate in ($candidates | Select-Object -Unique)) {
        $resolvedHome = [System.IO.Path]::GetFullPath($candidate)
        $versionText = Get-JavaVersionText $resolvedHome
        if ($versionText -match 'version\s+"21(?:\.|\")') {
            return [pscustomobject]@{ Home = $resolvedHome; VersionText = $versionText }
        }
    }
    throw 'JDK 21 not found. Pass -JavaHome or set JAVA_HOME to a JDK 21 installation.'
}

function Write-RunManifest {
    param([System.Collections.IDictionary]$Manifest, [string]$Path)
    $json = $Manifest | ConvertTo-Json -Depth 8
    $utf8 = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $json + [Environment]::NewLine, $utf8)
}

$binaryPath = Resolve-RepositoryPath $Binary
if (-not (Test-Path -LiteralPath $binaryPath -PathType Leaf)) {
    throw "Retail binary not found: $binaryPath"
}
$scriptPath = Resolve-RepositoryPath $Script
if (-not (Test-Path -LiteralPath $scriptPath -PathType Leaf)) {
    throw "Ghidra post-script not found: $scriptPath"
}
if ([System.IO.Path]::GetExtension($scriptPath) -ne '.java') {
    throw "Ghidra post-script must be a .java file: $scriptPath"
}

$outputPath = Resolve-RepositoryPath $OutputDirectory
$logsPrefix = $LogsRoot.TrimEnd('\', '/') + [System.IO.Path]::DirectorySeparatorChar
if (-not $outputPath.StartsWith($logsPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "OutputDirectory must be below $LogsRoot"
}
Assert-NoReparsePoint $outputPath $LogsRoot
if (Test-Path -LiteralPath $outputPath) {
    if (Get-ChildItem -LiteralPath $outputPath -Force | Select-Object -First 1) {
        throw "OutputDirectory must be new or empty: $outputPath"
    }
} else {
    New-Item -ItemType Directory -Path $outputPath -Force | Out-Null
}
Assert-NoReparsePoint $outputPath $LogsRoot

$reserved = @('JAVA_HOME', 'PATH', 'GHIDRA_HEADLESS_MAXMEM', 'GHIDRA_JAVA_OPTIONS', 'XIVL_DECOMP_ROOT')
foreach ($key in $ScriptEnvironment.Keys) {
    if ($reserved -contains [string]$key) {
        throw "ScriptEnvironment cannot override runner-owned variable: $key"
    }
}

$ghidra = Resolve-GhidraInstall $GhidraHome
$jdk = Resolve-Jdk21 $JavaHome
$headless = Join-Path $ghidra.Home 'support\analyzeHeadless.bat'
$projectDirectory = Join-Path $outputPath 'project'
$exportDirectory = Join-Path $outputPath 'exports'
$logPath = Join-Path $outputPath 'headless.log'
$manifestPath = Join-Path $outputPath 'run.json'
New-Item -ItemType Directory -Path $projectDirectory, $exportDirectory -Force | Out-Null

$scriptEnvironmentRecord = [ordered]@{}
foreach ($key in ($ScriptEnvironment.Keys | Sort-Object)) {
    $scriptEnvironmentRecord[[string]$key] = [string]$ScriptEnvironment[$key]
}
$binaryHash = (Get-FileHash -LiteralPath $binaryPath -Algorithm SHA256).Hash.ToLowerInvariant()
$scriptHash = (Get-FileHash -LiteralPath $scriptPath -Algorithm SHA256).Hash.ToLowerInvariant()
$started = [DateTimeOffset]::UtcNow
$manifest = [ordered]@{
    contract_version = 2
    status = 'running'
    analysis_timed_out = $false
    started_utc = $started.ToString('o')
    completed_utc = $null
    binary = [ordered]@{
        name = [System.IO.Path]::GetFileName($binaryPath)
        sha256 = $binaryHash
        path = $binaryPath
    }
    ghidra = [ordered]@{
        version = $ghidra.Version
        home = $ghidra.Home
    }
    jdk = [ordered]@{
        major_version = 21
        home = $jdk.Home
        version_text = $jdk.VersionText
    }
    script = [ordered]@{
        path = $scriptPath
        sha256 = $scriptHash
        arguments = @($ScriptArgument)
        environment = $scriptEnvironmentRecord
    }
    analysis = [ordered]@{
        mode = 'fresh-import'
        timeout_seconds = $AnalysisTimeoutSeconds
        max_memory = $MaxMemory
        program_writes_allowed = [bool]$AllowProgramWrites
    }
    outputs = [ordered]@{
        log = $logPath
        exports = $exportDirectory
        project = $projectDirectory
    }
    exit_code = $null
    elapsed_seconds = $null
    failure = $null
}
Write-RunManifest $manifest $manifestPath

$headlessArgs = @(
    (Convert-ToShortPath $projectDirectory),
    'evidence-run',
    '-import', (Convert-ToShortPath $binaryPath),
    '-scriptPath', (Convert-ToShortPath (Split-Path $scriptPath -Parent))
)
if ($AnalysisTimeoutSeconds -gt 0) {
    $headlessArgs += @('-analysisTimeoutPerFile', [string]$AnalysisTimeoutSeconds)
}
if (-not $AllowProgramWrites) {
    $headlessArgs += @('-readOnly', '-deleteProject')
}
$headlessArgs += @('-postScript', (Split-Path $scriptPath -Leaf))
$headlessArgs += @($ScriptArgument)

$saved = @{}
$applied = @{
    'JAVA_HOME' = $jdk.Home
    'PATH' = (Join-Path $jdk.Home 'bin') + ';' + $env:PATH
    'GHIDRA_HEADLESS_MAXMEM' = $MaxMemory
    'GHIDRA_JAVA_OPTIONS' = '-Dlog4j.skipJansi=true'
    'XIVL_DECOMP_ROOT' = $exportDirectory
}
foreach ($key in $ScriptEnvironment.Keys) {
    $applied[[string]$key] = [string]$ScriptEnvironment[$key]
}

$exitCode = 1
$failure = $null
$timer = [System.Diagnostics.Stopwatch]::StartNew()
try {
    foreach ($key in $applied.Keys) {
        $saved[$key] = [System.Environment]::GetEnvironmentVariable($key)
        Set-Item -Path "env:$key" -Value $applied[$key]
    }
    & (Convert-ToShortPath $headless) @headlessArgs *> $logPath
    $exitCode = $LASTEXITCODE
} catch {
    $failure = $_.Exception.Message
} finally {
    $timer.Stop()
    foreach ($key in $saved.Keys) {
        if ($null -eq $saved[$key]) {
            Remove-Item -Path "env:$key" -ErrorAction SilentlyContinue
        } else {
            Set-Item -Path "env:$key" -Value $saved[$key]
        }
    }
}

$logText = Get-Content -LiteralPath $logPath -Raw -ErrorAction SilentlyContinue
if ($null -eq $logText) { $logText = '' }
$analysisTimedOut = $logText -match 'Analysis timed out at \d+ seconds\. Processing not completed for file:'
$status = 'ok'
if ($failure) {
    $status = 'runner-error'
} elseif ($logText -match 'SCRIPT ERROR|Abort due to Headless analyzer error') {
    $status = 'script-error'
} elseif ($analysisTimedOut) {
    $status = 'analysis-timeout'
} elseif ($exitCode -ne 0) {
    $status = 'headless-error'
}
$manifest.status = $status
$manifest.analysis_timed_out = $analysisTimedOut
$manifest.completed_utc = [DateTimeOffset]::UtcNow.ToString('o')
$manifest.exit_code = $exitCode
$manifest.elapsed_seconds = [math]::Round($timer.Elapsed.TotalSeconds, 1)
$manifest.failure = $failure
Write-RunManifest $manifest $manifestPath

[pscustomobject]@{
    Status = $status
    Script = Split-Path $scriptPath -Leaf
    BinarySha256 = $binaryHash
    GhidraVersion = $ghidra.Version
    JdkMajorVersion = 21
    Seconds = $manifest.elapsed_seconds
    OutputDirectory = $outputPath
    Manifest = $manifestPath
    Log = $logPath
} | Format-List

if ($status -ne 'ok') {
    if ($failure) { Write-Error $failure }
    Select-String -LiteralPath $logPath -Pattern 'ERROR|Exception' -ErrorAction SilentlyContinue |
        Select-Object -First 8 |
        ForEach-Object { $_.Line.Trim() }
    exit 1
}
