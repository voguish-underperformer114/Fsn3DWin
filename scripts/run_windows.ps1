param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",
    [string]$BuildDir = "build",
    [string]$Root = "",
    [int]$MaxDepth = -1,
    [int]$MaxNodes = -1,
    [switch]$AutoScan
)

$ErrorActionPreference = "Stop"
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Push-Location $RepoRoot
try {
    $Exe = Join-Path $BuildDir "$Config\Fsn3DWin.exe"
    if (-not (Test-Path $Exe)) {
        & .\scripts\build_windows.ps1 -Config $Config -BuildDir $BuildDir
    }

    $AppArgs = @()
    if (-not [string]::IsNullOrWhiteSpace($Root)) {
        $AppArgs += @("--root", $Root)
    }
    if ($MaxDepth -ge 0) {
        $AppArgs += @("--max-depth", [string]$MaxDepth)
    }
    if ($MaxNodes -ge 0) {
        $AppArgs += @("--max-nodes", [string]$MaxNodes)
    }
    if ($AutoScan) {
        $AppArgs += "--auto-scan"
    }

    & $Exe @AppArgs
}
finally {
    Pop-Location
}
