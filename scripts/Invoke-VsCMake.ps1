param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("doctor", "configure", "build-debug", "build-release", "test-debug", "test-release")]
    [string]$Action
)

$ErrorActionPreference = "Stop"

# Windows environment blocks can contain keys that differ only by case (for
# example PATH/Path or HTTP_PROXY/http_proxy). MSBuild treats those keys as
# duplicates and fails before launching CL.exe. Collapse every duplicate group
# for this process only; user and machine environment settings remain unchanged.
$environmentEntries = & cmd.exe /d /c set | ForEach-Object {
    $separator = $_.IndexOf('=')
    if ($separator -gt 0) {
        [pscustomobject]@{
            Name = $_.Substring(0, $separator)
            Value = $_.Substring($separator + 1)
        }
    }
}

$duplicateEnvironmentGroups = $environmentEntries |
    Group-Object { $_.Name.ToLowerInvariant() } |
    Where-Object Count -gt 1

foreach ($group in $duplicateEnvironmentGroups) {
    $preferred = $group.Group |
        Where-Object { $_.Name -ceq $_.Name.ToUpperInvariant() } |
        Select-Object -First 1
    if (-not $preferred) {
        $preferred = $group.Group | Select-Object -First 1
    }

    $normalizedName = $group.Name
    foreach ($entry in $group.Group) {
        [Environment]::SetEnvironmentVariable($normalizedName, $null, [EnvironmentVariableTarget]::Process)
    }
    [Environment]::SetEnvironmentVariable($normalizedName, $preferred.Value, [EnvironmentVariableTarget]::Process)
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw "vswhere.exe was not found. Install Visual Studio with C++ CMake tools."
}

$visualStudioPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $visualStudioPath) {
    throw "A Visual Studio installation with the C++ toolchain was not found."
}

$visualStudioVersion = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion
if (-not $visualStudioVersion) {
    throw "The Visual Studio installation version could not be determined."
}

$visualStudioMajor = ([version]$visualStudioVersion).Major
$presetSuffix = switch ($visualStudioMajor) {
    18 { "vs2026" }
    17 { "vs2022" }
    default { throw "Visual Studio major version $visualStudioMajor is unsupported. Install Visual Studio 2022 or 2026." }
}
$configurePreset = "windows-$presetSuffix"
$debugBuildPreset = "debug-$presetSuffix"
$releaseBuildPreset = "release-$presetSuffix"
$buildDirectory = Join-Path "build" $configurePreset

$cmake = Join-Path $visualStudioPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if (-not (Test-Path -LiteralPath $cmake)) {
    throw "Visual Studio's bundled CMake was not found at: $cmake"
}

if ($Action -eq "doctor") {
    $git = Get-Command git -ErrorAction SilentlyContinue
    if (-not $git) {
        throw "Git was not found on PATH. Install Git for Windows."
    }

    Write-Output "Environment check passed."
    Write-Output "Visual Studio: $visualStudioVersion"
    Write-Output "Visual Studio path: $visualStudioPath"
    Write-Output "CMake preset: $configurePreset"
    & $cmake --version | Select-Object -First 1
    & git --version
    return
}

$arguments = switch ($Action) {
    "configure" { @("--preset", $configurePreset) }
    "build-debug" { @("--build", "--preset", $debugBuildPreset) }
    "build-release" { @("--build", "--preset", $releaseBuildPreset) }
    "test-debug" { @("--build", "--preset", $debugBuildPreset, "--target", "arcane_core_tests", "combat_session_tests", "run_flow_tests", "tower_session_tests") }
    "test-release" { @("--build", "--preset", $releaseBuildPreset, "--target", "arcane_core_tests", "combat_session_tests", "run_flow_tests", "tower_session_tests") }
}

& $cmake @arguments
if ($LASTEXITCODE -ne 0) {
    throw "CMake action '$Action' failed with exit code $LASTEXITCODE."
}

if ($Action -eq "test-debug" -or $Action -eq "test-release") {
    $configuration = if ($Action -eq "test-debug") { "Debug" } else { "Release" }
    & $cmake --build $buildDirectory --config $configuration --target RUN_TESTS
    if ($LASTEXITCODE -ne 0) {
        throw "Tests failed with exit code $LASTEXITCODE."
    }
}
