param(
    [string]$BuildDir = "build"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$Target = Join-Path $RepoRoot $BuildDir

if (-not (Test-Path $Target)) {
    Write-Host "Build directory does not exist: $Target"
    exit 0
}

$ResolvedTarget = Resolve-Path $Target
$RepoPrefix = $RepoRoot.Path.TrimEnd('\') + '\'
$TargetPath = $ResolvedTarget.Path

if (-not $TargetPath.StartsWith($RepoPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to remove path outside repo: $TargetPath"
}

if ([System.IO.Path]::GetFileName($TargetPath) -ne $BuildDir) {
    throw "Refusing to remove unexpected build path: $TargetPath"
}

Remove-Item -LiteralPath $TargetPath -Recurse -Force
Write-Host "Removed $TargetPath"
