param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,
    [string]$OutputDirectory = "release"
)

$ErrorActionPreference = "Stop"
$bridgeRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$bridgeExecutable = (Resolve-Path -LiteralPath $Executable).Path
if ([System.IO.Path]::IsPathRooted($OutputDirectory)) {
    $releaseDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
} else {
    $releaseDirectory = [System.IO.Path]::GetFullPath((Join-Path (Get-Location) $OutputDirectory))
}
$stagingDirectory = Join-Path $releaseDirectory "staging"

if (Test-Path -LiteralPath $stagingDirectory) {
    Remove-Item -LiteralPath $stagingDirectory -Recurse -Force
}
New-Item -ItemType Directory -Path $stagingDirectory -Force | Out-Null

$packages = @(
    @{
        Name = "ReplicazeronFusion"
        Source = Join-Path $bridgeRoot "fusion\ReplicazeronFusion"
    },
    @{
        Name = "ReplicazeronFreeCAD"
        Source = Join-Path $bridgeRoot "freecad\ReplicazeronFreeCAD"
    }
)

foreach ($package in $packages) {
    $packageDirectory = Join-Path $stagingDirectory $package.Name
    Copy-Item -LiteralPath $package.Source -Destination $packageDirectory -Recurse
    Get-ChildItem -LiteralPath $packageDirectory -Directory -Filter "__pycache__" -Recurse |
        Remove-Item -Recurse -Force
    Copy-Item -LiteralPath $bridgeExecutable -Destination (Join-Path $packageDirectory "ReplicazeronCadBridge.exe")
    Copy-Item -LiteralPath (Join-Path $bridgeRoot "README.md") -Destination (Join-Path $packageDirectory "README.md")
    if (-not (Test-Path -LiteralPath (Join-Path $packageDirectory "LICENSE"))) {
        Copy-Item -LiteralPath (Join-Path $bridgeRoot "LICENSE") -Destination (Join-Path $packageDirectory "LICENSE")
    }

    $archive = Join-Path $releaseDirectory ($package.Name + "-windows.zip")
    if (Test-Path -LiteralPath $archive) {
        Remove-Item -LiteralPath $archive -Force
    }
    Compress-Archive -LiteralPath $packageDirectory -DestinationPath $archive -CompressionLevel Optimal
}

Copy-Item -LiteralPath $bridgeExecutable -Destination (Join-Path $releaseDirectory "ReplicazeronCadBridge.exe") -Force
Remove-Item -LiteralPath $stagingDirectory -Recurse -Force
