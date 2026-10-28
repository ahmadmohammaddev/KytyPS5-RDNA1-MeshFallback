# Windows PowerShell 5.1+; package-local verification and synthetic-test runner.
[CmdletBinding()]
param(
    [ValidateSet('Offline', 'Strict', 'Vulkan12')][string]$Mode = 'Strict',
    [string]$ValidationLayerDirectory,
    [string]$ResultsDirectory,
    [switch]$RequireRx5700Xt,
    [switch]$VerifyOnly,
    [switch]$Draw,
    [switch]$Texture,
    [switch]$IsolateImplicitLayers
)
$ErrorActionPreference = 'Stop'
if ($Texture) { $Draw = $true }
$root = [IO.Path]::GetFullPath($PSScriptRoot)
function FileHash([string]$Path) {
    $stream = [IO.File]::OpenRead($Path)
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '') }
    finally { $sha.Dispose(); $stream.Dispose() }
}
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
    # Windows PowerShell 5.1 emits a JSON array as one pipeline object; normalize
    # after assignment rather than wrapping that pipeline (which makes it nested).
    $checks = Get-Content -Raw -LiteralPath (Join-Path $root 'SHA256SUMS.json') | ConvertFrom-Json
    $checks = @($checks)
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
        if ((FileHash $path) -ne $entry.sha256) {
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
$oldImplicitLayerPath = $env:VK_IMPLICIT_LAYER_PATH
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
    if ($IsolateImplicitLayers) {
        # Ordinary-user process override only. Keep explicit Khronos validation;
        # do not edit registry/ICD discovery. Elevation may ignore this override.
        $implicitDirectory = Join-Path $results 'empty-implicit-layers'
        New-Item -ItemType Directory -Path $implicitDirectory | Out-Null
        $env:VK_IMPLICIT_LAYER_PATH = $implicitDirectory
    }
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
    if ($preflightOk -and $Draw) {
        foreach ($argument in @('--check-draw-oracle','--validate-draw-spirv')) {
            $name = if ($argument -eq '--check-draw-oracle') { 'draw-oracle' } else { 'draw-spirv' }
            $run = RunCheck -Name $name -Exe 'mesh_compute_proto_tests.exe' -Arguments @($argument)
            if ($run.record.exit -ne 0) { $preflightOk = $false; break }
        }
    }
    if ($preflightOk -and $Texture) {
        foreach ($argument in @('--check-texture-fixtures','--validate-texture-spirv')) {
            $name = if ($argument -eq '--check-texture-fixtures') { 'texture-fixtures' } else { 'texture-spirv' }
            $run = RunCheck -Name $name -Exe 'mesh_compute_proto_tests.exe' -Arguments @($argument)
            if ($run.record.exit -ne 0) { $preflightOk = $false; break }
        }
    }
    $runtimeStatus = 'NOT RUN'
    $runtimeIdentity = @()
    $overall = if ($preflightOk) { 'PASS' } else { 'FAIL' }
    if ($preflightOk -and $Mode -ne 'Offline') {
        $argument = if ($Mode -eq 'Strict') { '--run' } else { '--run-vulkan12' }
        $arguments = @($argument)
        if ($Draw) {
            $argument = if ($Mode -eq 'Strict') { '--draw' } else { '--draw-vulkan12' }
            if ($Texture) { $argument = if ($Mode -eq 'Strict') { '--texture' } else { '--texture-vulkan12' } }
            $arguments = @($argument, '--image-output', (Join-Path $results 'triangle.ppm'))
        }
        $run = RunCheck -Name 'runtime' -Exe 'mesh_compute_proto_tests.exe' -Arguments $arguments
        $runtimeStatus = $run.record.status
        $runtimeIdentity = @($run.lines | Where-Object { $_ -match 'physical-devices:|\[\d+\].*type=|selected:|selected-driver:|dispatch-module:|summary after cleanup:|offscreen.*pixels|DRAW RESULT:|TEXTURE RESULT:|UV boundary|UV full-buffer:' })
        $overall = $runtimeStatus
        if ($RequireRx5700Xt -and $runtimeStatus -eq 'PASS' -and
            -not (@($run.lines | Where-Object { $_ -match 'selected:.*RX\s*5700\s*XT' }).Count)) {
            $overall = 'FAIL'
            Write-Host 'FAIL: runtime completed but the selected device was not identified as RX 5700 XT'
        }
    }
    $summary = [ordered]@{schema=1; gitCommit=$manifest.gitCommit; buildId=$manifest.buildId;
        mode=$Mode; draw=[bool]$Draw; texture=[bool]$Texture; isolateImplicitLayers=[bool]$IsolateImplicitLayers; requireRx5700Xt=[bool]$RequireRx5700Xt; status=$overall; runtime=$runtimeStatus;
        runs=@($runs.ToArray()); deviceEvidence=$runtimeIdentity;
        environment=@{VK_LAYER_PATH=$env:VK_LAYER_PATH; VK_INSTANCE_LAYERS=$env:VK_INSTANCE_LAYERS;
            VK_DRIVER_FILES=$env:VK_DRIVER_FILES; VK_ICD_FILENAMES=$env:VK_ICD_FILENAMES;
            VK_IMPLICIT_LAYER_PATH=$env:VK_IMPLICIT_LAYER_PATH; VK_ADD_IMPLICIT_LAYER_PATH=$env:VK_ADD_IMPLICIT_LAYER_PATH};
        rdna1Evidence='Only a separately identified native RX 5700 XT strict run may qualify';
        image=$(if ($Draw -and (Test-Path -LiteralPath (Join-Path $results 'triangle.ppm'))) {
            @{file='triangle.ppm'; sha256=(FileHash (Join-Path $results 'triangle.ppm'))}
        } else { $null });
        untexturedImage=$(if ($Texture -and (Test-Path -LiteralPath (Join-Path $results 'untextured.ppm'))) {
            @{file='untextured.ppm'; sha256=(FileHash (Join-Path $results 'untextured.ppm'))}
        } else { $null }); astroBot='NOT RUN'}
    $summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $results 'result.json') -Encoding UTF8
    Write-Host "RESULT: $overall; runtime: $runtimeStatus; logs: $results"
    $exitCode = if ($overall -eq 'PASS') { 0 } elseif ($overall -eq 'SKIPPED') { 41 } else { 1 }
} catch {
    Write-Host "FAIL: runner: $_"
    $exitCode = 1
} finally {
    $env:VK_LAYER_PATH = $oldLayerPath
    $env:VK_IMPLICIT_LAYER_PATH = $oldImplicitLayerPath
}
exit $exitCode
