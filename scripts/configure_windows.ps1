param(
    [string]$BuildDir = "build",
    [string]$Generator = "Visual Studio 17 2022",
    [string]$Architecture = "x64",
    [string]$VcpkgRoot = ""
)

$ErrorActionPreference = "Stop"
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Push-Location $RepoRoot
try {
    if ([string]::IsNullOrWhiteSpace($VcpkgRoot)) {
        if ($env:VCPKG_ROOT) {
            $VcpkgRoot = $env:VCPKG_ROOT
        } elseif (Test-Path ".tools\vcpkg\scripts\buildsystems\vcpkg.cmake") {
            $VcpkgRoot = ".tools\vcpkg"
        } else {
            throw "Set VCPKG_ROOT or place vcpkg at .tools\vcpkg."
        }
    }

    $Toolchain = Join-Path $VcpkgRoot "scripts\buildsystems\vcpkg.cmake"
    if (-not (Test-Path $Toolchain)) {
        throw "vcpkg toolchain file not found: $Toolchain"
    }

    cmake -S . -B $BuildDir -G $Generator -A $Architecture -DCMAKE_TOOLCHAIN_FILE="$Toolchain"
}
finally {
    Pop-Location
}
