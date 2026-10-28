# Build/package only the isolated synthetic test; requires an x64 VS developer shell.
[CmdletBinding()]
param(
    [string]$BuildDirectory = '_Build/windows-core',
    [string]$OutputDirectory = '_Build/mesh-prototype-packages',
    [string]$ValidationLayerDirectory,
    [ValidateSet(1,2)][int]$Jobs = 2
)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
function FileHash([string]$Path) {
    $stream = [IO.File]::OpenRead($Path)
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '') }
    finally { $sha.Dispose(); $stream.Dispose() }
}
function BuildPath([string]$Relative) {
    $path = if ([IO.Path]::IsPathRooted($Relative)) { [IO.Path]::GetFullPath($Relative) } else {
        [IO.Path]::GetFullPath((Join-Path $repo $Relative))
    }
    $allowed = [IO.Path]::GetFullPath((Join-Path $repo '_Build')) + [IO.Path]::DirectorySeparatorChar
    if (-not $path.StartsWith($allowed, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Build/output paths must remain inside this checkout/_Build'
    }
    return $path
}
function GitLines([string[]]$Arguments) {
    $lines = @(& git -C $repo @Arguments)
    if ($LASTEXITCODE -ne 0) { throw "Git failed: $Arguments" }
    return $lines
}
function AssertClean {
    if (@(GitLines -Arguments @('status','--porcelain=v1')).Count) { throw 'Packaging requires clean committed source' }
    $states = @(GitLines -Arguments @('submodule','status','--recursive'))
    foreach ($state in $states) {
        if ($state -notmatch '^ ([0-9a-f]{40}) (\S+)') { throw 'Submodule revision mismatch or uninitialized submodule' }
        if (@(GitLines -Arguments @('-C',$Matches[2],'status','--porcelain=v1')).Count) { throw 'Dirty submodule' }
    }
    return $states
}
function Native([string]$Exe, [string[]]$Arguments, [string]$Log) {
    $savedPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $global:LASTEXITCODE = $null
        $lines = @(& $Exe @Arguments 2>&1 | ForEach-Object { "$_" })
        $code = $LASTEXITCODE
    } finally { $ErrorActionPreference = $savedPreference }
    $lines | Set-Content -LiteralPath $Log -Encoding UTF8
    if ($null -eq $code -or $code -ne 0) { throw "Command failed ($code): $Exe; retained log $Log" }
    return $lines
}
function CopyPayload([string]$Source, [string]$Relative) {
    $destination = Join-Path $stage $Relative
    New-Item -ItemType Directory -Force -Path (Split-Path $destination) | Out-Null
    Copy-Item -LiteralPath $Source -Destination $destination
}
function FileRecord([IO.FileInfo]$File) {
    return [ordered]@{path=$File.FullName.Substring($stage.Length+1).Replace([char]92,[char]47);
        bytes=$File.Length; sha256=(FileHash $File.FullName)}
}
$oldLayerPath = $env:VK_LAYER_PATH
$oldInstanceLayers = $env:VK_INSTANCE_LAYERS
try {
    $submodules = @(AssertClean)
    GitLines -Arguments @('merge-base','--is-ancestor','u59-windows-20261024-int21','HEAD') | Out-Null
    # PowerShell unwraps single pipeline strings: normalize without indexing characters.
    $commit = [string]::Join('', @(GitLines -Arguments @('rev-parse','HEAD')))
    $branch = [string]::Join('', @(GitLines -Arguments @('branch','--show-current')))
    $build = BuildPath $BuildDirectory
    $output = BuildPath $OutputDirectory
    $buildId = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')
    $name = "Kyty-MeshPrototype-Windows-x64-$commit-$buildId"
    $evidence = Join-Path $output "$name-evidence"
    $stage = Join-Path $output $name
    if ((Test-Path -LiteralPath $stage) -or (Test-Path -LiteralPath $evidence)) { throw 'Build ID already exists' }
    New-Item -ItemType Directory -Force -Path $evidence | Out-Null
    $submodules | Set-Content -LiteralPath (Join-Path $evidence 'submodules.txt') -Encoding UTF8
    $started = [DateTime]::UtcNow.ToString('o')
    $cmake = (Get-Command cmake -ErrorAction Stop).Source
    $ctest = (Get-Command ctest -ErrorAction Stop).Source
    $compiler = (Get-Command clang-cl -ErrorAction Stop).Source
    $dumpbin = (Get-Command dumpbin -ErrorAction Stop).Source
    $cmakeVersion = @(Native -Exe $cmake -Arguments @('--version') -Log "$evidence/cmake-version.log")
    $compilerVersion = @(Native -Exe $compiler -Arguments @('--version') -Log "$evidence/compiler-version.log")
    if (-not ($compilerVersion -match 'Target: x86_64.*windows')) { throw 'Use an x64 Windows clang-cl developer environment' }
    $ninjaVersion = @(Native -Exe (Get-Command ninja).Source -Arguments @('--version') -Log "$evidence/ninja-version.log")
    $flags = @('-DCMAKE_BUILD_TYPE=Release','-DCMAKE_C_COMPILER=clang-cl','-DCMAKE_CXX_COMPILER=clang-cl',
        '-DKYTY_BUILD_LAUNCHER=OFF','-DBUILD_TESTING=ON','-DKYTY_PGO_GENERATE=OFF',
        '-DKYTY_PGO_USE:FILEPATH=','-DKYTY_EMULATOR_IPO=OFF','-DKYTY_MARCH:STRING=')
    Write-Host "Configure/build clean commit $commit (jobs=$Jobs); evidence: $evidence"
    Native -Exe $cmake -Arguments (@('-S',$repo,'-B',$build,'-G','Ninja') + $flags) -Log "$evidence/configure.log" | Out-Null
    Native -Exe $cmake -Arguments @('--build',$build,'--target','KytyGitVersion','--parallel',"$Jobs") -Log "$evidence/version-build.log" | Out-Null
    Native -Exe $cmake -Arguments @('--build',$build,'--target','mesh_output_contract_tests','mesh_compute_proto_tests','--parallel',"$Jobs") -Log "$evidence/build.log" | Out-Null
    Copy-Item -LiteralPath "$build/CMakeCache.txt" -Destination "$evidence/CMakeCache.txt"
    Copy-Item -LiteralPath "$build/kytyGitVersion.h" -Destination "$evidence/kytyGitVersion.h"
    $cache = Get-Content -LiteralPath "$build/CMakeCache.txt"
    $shaderCompilerLine = @($cache | Where-Object { $_ -match '^KYTY_GLSLANG_VALIDATOR:FILEPATH=' })
    if ($shaderCompilerLine.Count -ne 1) { throw 'Shader compiler missing from configured cache' }
    $shaderCompiler = $shaderCompilerLine[0].Substring($shaderCompilerLine[0].IndexOf('=')+1)
    $glslangVersion = @(Native -Exe $shaderCompiler -Arguments @('--version') -Log "$evidence/glslang-version.log")
    if ($ValidationLayerDirectory) {
        $layer = [IO.Path]::GetFullPath($ValidationLayerDirectory)
        if (-not (Test-Path -LiteralPath (Join-Path $layer 'VkLayer_khronos_validation.json'))) { throw 'Invalid validation layer directory' }
        $env:VK_LAYER_PATH = $layer
    }
    $env:VK_INSTANCE_LAYERS = $null # the harness explicitly enables its required layer
    $selection = '^(mesh_output_contract|mesh_proto_spirv|mesh_proto_host_fixtures|mesh_proto_reject_shape|mesh_compute_proto|mesh_compute_proto_vk12|mesh_draw_oracle|mesh_draw_spirv|mesh_offscreen_draw|mesh_offscreen_draw_vk12|mesh_texture_fixtures|mesh_texture_spirv|mesh_textured_draw|mesh_textured_draw_vk12)$'
    Native -Exe $ctest -Arguments @('--test-dir',$build,'--output-on-failure','--parallel','1','-V',
        '--output-junit',"$evidence/ctest.xml",'-R',$selection) -Log "$evidence/ctest.log" | Out-Null
    [xml]$junit = Get-Content -Raw -LiteralPath "$evidence/ctest.xml"
    if ([int]$junit.testsuite.tests -ne 14 -or [int]$junit.testsuite.failures -ne 0) { throw 'Unexpected test inventory/results' }
    $testResults = @($junit.testsuite.testcase | ForEach-Object {
        [ordered]@{name=$_.name; status=$(if ($_.skipped) { 'SKIPPED' } else { 'PASS' })}
    })
    AssertClean | Out-Null
    if ([string]::Join('', @(GitLines -Arguments @('rev-parse','HEAD'))) -ne $commit) { throw 'Commit changed during build' }
    New-Item -ItemType Directory -Path $stage | Out-Null
    foreach ($file in @('mesh_output_contract_tests.exe','mesh_compute_proto_tests.exe')) { CopyPayload "$build/$file" $file }
    foreach ($file in @('mesh_output_proto_vk12.spv','mesh_output_proto_vk13.spv','mesh_draw_vert_vk12.spv','mesh_draw_frag_vk12.spv','mesh_draw_vert_vk13.spv','mesh_draw_frag_vk13.spv')) {
        CopyPayload "$build/gpu_mesh_proto_shaders/$file" "gpu_mesh_proto_shaders/$file"
    }
    foreach ($api in @('12','13')) {
        foreach ($stageName in @('comp','vert','frag')) {
            $file = "mesh_texture_${stageName}_vk${api}.spv"
            CopyPayload "$build/gpu_mesh_proto_shaders/$file" "gpu_mesh_proto_shaders/$file"
        }
    }
    foreach ($file in @('Run-Prototype.ps1','Run-Strict.cmd','Run-Draw.cmd','Run-Textured.cmd','README.md','THIRD_PARTY_NOTICES.md')) {
        CopyPayload "$PSScriptRoot/$file" $file
    }
    CopyPayload "$repo/LICENSE" 'LICENSE'
    CopyPayload "$repo/LICENSES/Kyty-MIT.txt" 'LICENSES/Kyty-MIT.txt'
    CopyPayload "$repo/3rdparty/SPIRV-Tools/LICENSE" 'LICENSES/SPIRV-Tools.txt'
    CopyPayload "$repo/3rdparty/SPIRV-Headers/LICENSE" 'LICENSES/SPIRV-Headers.txt'
    foreach ($file in @('MeshComputePrototypeTests.cpp','MeshOutputContractTests.cpp','MeshOutputContract.h','MeshOffscreenDraw.h','MeshTextureProbe.h')) {
        CopyPayload "$repo/tests/$file" "source/$file"
    }
    foreach ($shader in @('mesh_output_proto.comp','mesh_draw.vert','mesh_draw.frag','mesh_uv.comp','mesh_texture.vert','mesh_texture.frag')) {
        CopyPayload "$repo/tests/shaders/$shader" "source/$shader"
    }
    $imports = @()
    foreach ($file in @('mesh_compute_proto_tests.exe','mesh_output_contract_tests.exe')) {
        $inspection = @(Native -Exe $dumpbin -Arguments @('/DEPENDENTS',"$stage/$file") -Log "$evidence/$file-imports.log")
        foreach ($line in $inspection) {
            if ($line -match '^\s+([A-Za-z0-9_.-]+\.dll)\s*$') { $imports += $Matches[1] }
        }
    }
    $imports = @($imports | Sort-Object -Unique)
    if (-not $imports.Count) { throw 'PE import inspection returned no dependencies' }
    foreach ($dll in $imports) {
        if ($dll -notmatch '^(KERNEL32|MSVCP140|VCRUNTIME140(?:_1)?|ucrtbase|api-ms-win-crt-[a-z0-9-]+)\.dll$') {
            throw "Unaccounted runtime dependency: $dll"
        }
    }
    $manifest = [ordered]@{schema=1; artifact='Standalone synthetic compute and offscreen draw test, not an emulator';
        gitCommit=$commit; branch=$branch; dirty=$false; buildId=$buildId; startedUtc=$started;
        completedUtc=[DateTime]::UtcNow.ToString('o'); submodules=$submodules;
        toolchain=@{cmake=$cmakeVersion[0]; compiler=$compilerVersion[0]; target='Windows x64';
            ninja=$ninjaVersion[0]; glslang=$glslangVersion[0]; msvcHeaders=$env:VCToolsVersion; windowsSdk=$env:WindowsSDKVersion};
        configuration=@{generator='Ninja'; flags=$flags; jobs=$Jobs; testParallelism=1};
        commands=@('cmake -S SOURCE -B BUILD -G Ninja [configuration.flags]',
            'cmake --build BUILD --target KytyGitVersion --parallel JOBS',
            'cmake --build BUILD --target mesh_output_contract_tests mesh_compute_proto_tests --parallel JOBS',
            "ctest --test-dir BUILD --output-on-failure --parallel 1 -V -R $selection");
        buildTests=$testResults; runtimeImports=$imports;
        externalPrerequisites=@('Windows 10/11 x64','Microsoft Visual C++ v14 x64 runtime (at least build tool version)',
            'Native GPU Vulkan driver; strict test requires Vulkan 1.3','Khronos validation layer with synchronization validation');
        validationLayerBundled=$false; rdna1='NOT RUN'; astroBot='NOT RUN';
        files=@(Get-ChildItem -LiteralPath $stage -Recurse -File | Sort-Object FullName | ForEach-Object { FileRecord $_ })}
    $manifest | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath "$stage/manifest.json" -Encoding UTF8
    @(Get-ChildItem -LiteralPath $stage -Recurse -File | Sort-Object FullName | ForEach-Object { FileRecord $_ }) |
        ConvertTo-Json -Depth 4 | Set-Content -LiteralPath "$stage/SHA256SUMS.json" -Encoding UTF8
    Native -Exe 'powershell.exe' -Arguments @('-NoProfile','-ExecutionPolicy','Bypass','-File',"$stage/Run-Prototype.ps1",'-VerifyOnly') -Log "$evidence/package-verification.log" | Out-Null
    AssertClean | Out-Null
    if ([string]::Join('', @(GitLines -Arguments @('rev-parse','HEAD'))) -ne $commit) { throw 'Commit changed during staging' }
    $archive = Join-Path $output "$name.zip"
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [IO.Compression.ZipFile]::CreateFromDirectory($stage, $archive, [IO.Compression.CompressionLevel]::Optimal, $true)
    $archiveHash = FileHash $archive
    "$archiveHash  $name.zip" | Set-Content -LiteralPath "$archive.sha256" -Encoding ASCII
    [ordered]@{archive=$archive; sha256=$archiveHash; gitCommit=$commit; evidence=$evidence; stage=$stage} |
        ConvertTo-Json | Set-Content -LiteralPath "$evidence/package-result.json" -Encoding UTF8
    Write-Host "PACKAGE: $archive"
    Write-Host "SHA256: $archiveHash"
    Write-Host 'Native RX validation and desktop deployment are NOT RUN.'
} finally {
    $env:VK_LAYER_PATH = $oldLayerPath
    $env:VK_INSTANCE_LAYERS = $oldInstanceLayers
}
