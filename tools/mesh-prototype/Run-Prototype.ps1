# Windows PowerShell 5.1+; package-local verification and synthetic-test runner.
[CmdletBinding()]
param(
    [ValidateSet('Offline', 'Strict', 'Vulkan12')][string]$Mode = 'Strict',
    [string]$ValidationLayerDirectory,
    [string]$ResultsDirectory,
    [switch]$RequireRx5700Xt,
    [switch]$VerifyOnly
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath($PSScriptRoot)
function PackagePath([string]$Relative) {
    if ([IO.Path]::IsPathRooted($Relative) -or $Relative -match '(^|[\\/])\.\.([\\/]|$)') {
        throw "Invalid package path: $Relative"
    }
    $path = [IO.Path]::GetFullPath((Join-Path $root $Relative))
    if (-not $path.StartsWith($root + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Path escapes package: $Relative"
    }
    return $path
}
try {
    $manifest = Get-Content -Raw -LiteralPath (Join-Path $root 'manifest.json') | ConvertFrom-Json
    if ($manifest.schema -ne 1 -or $manifest.gitCommit -notmatch '^[0-9a-f]{40}$' -or $manifest.dirty -ne $false) {
        throw 'Unsupported or dirty package identity'
    }
    $checks = @(Get-Content -Raw -LiteralPath (Join-Path $root 'SHA256SUMS.json') | ConvertFrom-Json)
    $expected = @($manifest.files | ForEach-Object { $_.path }) + @('manifest.json')
    if ($checks.Count -ne $expected.Count) { throw 'Checksum inventory does not match manifest' }
    $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($entry in $checks) {
        if (-not $seen.Add($entry.path) -or $expected -notcontains $entry.path -or $entry.sha256 -notmatch '^[0-9A-Fa-f]{64}$') {
            throw 'Unexpected or duplicate checksum entry'
        }
        $path = PackagePath $entry.path
        $item = Get-Item -LiteralPath $path
        if ($item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw 'Expected ordinary package file'
        }
        if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $entry.sha256) {
            throw "Checksum mismatch: $($entry.path)"
        }
    }
    $actual = @(Get-ChildItem -LiteralPath $root -Recurse -Force -File)
    if ($actual.Count -ne $checks.Count + 1) { throw 'Unexpected files in package; keep results outside it' }
    foreach ($directory in @(Get-ChildItem -LiteralPath $root -Recurse -Force -Directory)) {
        if ($directory.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Linked directories are not allowed' }
    }
    Write-Host "Package verified: $($manifest.gitCommit) / $($manifest.buildId)"
} catch {
    Write-Host "FAIL: package verification: $_"
    exit 1
}
if ($VerifyOnly) { exit 0 }

$oldLayerPath = $env:VK_LAYER_PATH
$exitCode = 1
try {
    if ($ValidationLayerDirectory) {
        $layer = [IO.Path]::GetFullPath($ValidationLayerDirectory)
        if (-not (Test-Path -LiteralPath (Join-Path $layer 'VkLayer_khronos_validation.json') -PathType Leaf)) {
            throw 'ValidationLayerDirectory must contain VkLayer_khronos_validation.json'
        }
        $env:VK_LAYER_PATH = $layer
    }
    if (-not $ResultsDirectory) {
        $base = [Environment]::GetFolderPath('LocalApplicationData')
        $ResultsDirectory = Join-Path $base ('KytyMeshPrototypeResults/' + $manifest.gitCommit + '/' +
            [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ') + '-' + [Guid]::NewGuid().ToString('N').Substring(0,8))
    }
    $results = [IO.Path]::GetFullPath($ResultsDirectory)
    if ($results.Equals($root, [StringComparison]::OrdinalIgnoreCase) -or
        $results.StartsWith($root + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'ResultsDirectory must be outside the verified package'
    }
    if (Test-Path -LiteralPath $results) { throw 'ResultsDirectory must be new; existing evidence is never overwritten' }
    New-Item -ItemType Directory -Path $results -Force | Out-Null
    $runs = [Collections.Generic.List[object]]::new()
    function RunCheck([string]$Name, [string]$Exe, [string[]]$Arguments) {
        $started = [DateTime]::UtcNow
        $savedPreference = $ErrorActionPreference
        try {
            $ErrorActionPreference = 'Continue'
            $global:LASTEXITCODE = $null
            $lines = @(& (PackagePath $Exe) @Arguments 2>&1 | ForEach-Object { "$_" })
            $code = if ($null -eq $LASTEXITCODE) { -1 } else { $LASTEXITCODE }
        } finally { $ErrorActionPreference = $savedPreference }
        $lines | Set-Content -LiteralPath (Join-Path $results "$Name.log") -Encoding UTF8
        $status = if ($code -eq 0) { 'PASS' } elseif ($code -eq 41) { 'SKIPPED' } else { 'FAIL' }
        $record = [pscustomobject]@{name=$Name; arguments=$Arguments; exit=$code; status=$status;
            startUtc=$started.ToString('o'); durationSeconds=([DateTime]::UtcNow-$started).TotalSeconds}
        $runs.Add($record)
        Write-Host "$Name : $status (exit $code)"
        return [pscustomobject]@{record=$record; lines=$lines}
    }
    $preflightOk = $true
    foreach ($check in @(
        @{name='host-contract'; exe='mesh_output_contract_tests.exe'; arguments=@()},
        @{name='host-fixtures'; exe='mesh_compute_proto_tests.exe'; arguments=@('--check-host-fixtures')},
        @{name='spirv'; exe='mesh_compute_proto_tests.exe'; arguments=@('--validate-spirv')}
    )) {
        $run = RunCheck -Name $check.name -Exe $check.exe -Arguments $check.arguments
        if ($run.record.exit -ne 0) { $preflightOk = $false; break }
    }
    $runtimeStatus = 'NOT RUN'
    $runtimeIdentity = @()
    $overall = if ($preflightOk) { 'PASS' } else { 'FAIL' }
    if ($preflightOk -and $Mode -ne 'Offline') {
        $argument = if ($Mode -eq 'Strict') { '--run' } else { '--run-vulkan12' }
        $run = RunCheck -Name 'runtime' -Exe 'mesh_compute_proto_tests.exe' -Arguments @($argument)
        $runtimeStatus = $run.record.status
        $runtimeIdentity = @($run.lines | Where-Object { $_ -match 'physical-devices:|\[\d+\].*type=|selected:|selected-driver:|dispatch-module:|summary after cleanup:' })
        $overall = $runtimeStatus
        if ($RequireRx5700Xt -and $runtimeStatus -eq 'PASS' -and
            -not (@($run.lines | Where-Object { $_ -match 'selected:.*RX\s*5700\s*XT' }).Count)) {
            $overall = 'FAIL'
            Write-Host 'FAIL: runtime completed but the selected device was not identified as RX 5700 XT'
        }
    }
    $summary = [ordered]@{schema=1; gitCommit=$manifest.gitCommit; buildId=$manifest.buildId;
        mode=$Mode; requireRx5700Xt=[bool]$RequireRx5700Xt; status=$overall; runtime=$runtimeStatus;
        runs=@($runs.ToArray()); deviceEvidence=$runtimeIdentity;
        environment=@{VK_LAYER_PATH=$env:VK_LAYER_PATH; VK_INSTANCE_LAYERS=$env:VK_INSTANCE_LAYERS;
            VK_DRIVER_FILES=$env:VK_DRIVER_FILES; VK_ICD_FILENAMES=$env:VK_ICD_FILENAMES};
        rdna1Evidence='Only a separately identified native RX 5700 XT strict run may qualify';
        astroBot='NOT RUN'}
    $summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $results 'result.json') -Encoding UTF8
    Write-Host "RESULT: $overall; runtime: $runtimeStatus; logs: $results"
    $exitCode = if ($overall -eq 'PASS') { 0 } elseif ($overall -eq 'SKIPPED') { 41 } else { 1 }
} catch {
    Write-Host "FAIL: runner: $_"
    $exitCode = 1
} finally { $env:VK_LAYER_PATH = $oldLayerPath }
exit $exitCode
