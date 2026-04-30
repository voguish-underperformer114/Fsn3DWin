param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",
    [string]$BuildDir = "build"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Push-Location $RepoRoot
try {
    if (-not (Test-Path (Join-Path $BuildDir "CMakeCache.txt"))) {
        & .\scripts\configure_windows.ps1 -BuildDir $BuildDir
    }

    cmake --build $BuildDir --config $Config
}
finally {
    Pop-Location
}
