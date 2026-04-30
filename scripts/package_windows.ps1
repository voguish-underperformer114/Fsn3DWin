param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",
    [string]$BuildDir = "build",
    [string]$Version = "0.3.0"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Push-Location $RepoRoot
try {
    & .\scripts\build_windows.ps1 -Config $Config -BuildDir $BuildDir

    $DistRoot = Join-Path $RepoRoot "dist"
    $PackageName = "Fsn3DWin-$Version-windows-x64"
    $PackageDir = Join-Path $DistRoot $PackageName
    $ZipPath = Join-Path $DistRoot "$PackageName.zip"

    if (Test-Path $PackageDir) {
        Remove-Item -LiteralPath $PackageDir -Recurse -Force
    }
    if (Test-Path $ZipPath) {
        Remove-Item -LiteralPath $ZipPath -Force
    }

    New-Item -ItemType Directory -Path $PackageDir | Out-Null

    $BinaryDir = Join-Path $BuildDir $Config
    Copy-Item -LiteralPath (Join-Path $BinaryDir "Fsn3DWin.exe") -Destination $PackageDir

    Get-ChildItem -Path $BinaryDir -Filter "*.dll" -File | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $PackageDir
    }

    Copy-Item -LiteralPath "README.md" -Destination $PackageDir
    Copy-Item -LiteralPath "vcpkg.json" -Destination $PackageDir

    Compress-Archive -Path (Join-Path $PackageDir "*") -DestinationPath $ZipPath -CompressionLevel Optimal
    Write-Host "Created $ZipPath"
}
finally {
    Pop-Location
}
