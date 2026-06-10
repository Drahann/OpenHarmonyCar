param (
    [ValidateSet("shared", "static")]
    [string]$STL = "shared",

    [ValidateSet("v8", "v7")]
    [string]$Arch = "v8",

    [string]$NativePath = "D:\Project\Harmony\ohos-sdk-5.0\windows\native",

    [string]$BuildDir = "build"
)

$ErrorActionPreference = "Stop"

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildPath = Join-Path $scriptRoot $BuildDir
$rootPath = (Resolve-Path -LiteralPath $scriptRoot).Path

if ($STL -eq "shared") {
    $ohosStl = "c++_shared"
}
else {
    $ohosStl = "c++_static"
}

if ($Arch -eq "v8") {
    $ohosArch = "arm64-v8a"
}
else {
    $ohosArch = "armeabi-v7a"
}

$cmakePath = Join-Path $NativePath "build-tools\cmake\bin\cmake.exe"
$ninjaPath = Join-Path $NativePath "build-tools\cmake\bin\ninja.exe"
$toolchainPath = Join-Path $NativePath "build\cmake\ohos.toolchain.cmake"
$clangPath = Join-Path $NativePath "llvm\bin\clang.exe"
$clangxxPath = Join-Path $NativePath "llvm\bin\clang++.exe"

foreach ($path in @($cmakePath, $ninjaPath, $toolchainPath, $clangPath, $clangxxPath)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Required tool not found: $path"
    }
}

if (Test-Path -LiteralPath $buildPath) {
    $resolvedBuild = (Resolve-Path -LiteralPath $buildPath).Path
    if (-not $resolvedBuild.StartsWith($rootPath, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove build directory outside project: $resolvedBuild"
    }
    Remove-Item -LiteralPath $resolvedBuild -Recurse -Force
}

New-Item -ItemType Directory -Path $buildPath | Out-Null

Push-Location $buildPath
try {
    & $cmakePath `
        -D CMAKE_C_COMPILER="$clangPath" `
        -D CMAKE_CXX_COMPILER="$clangxxPath" `
        -D OHOS_STL="$ohosStl" `
        -D OHOS_ARCH="$ohosArch" `
        -D OHOS_PLATFORM=OHOS `
        -D CMAKE_TOOLCHAIN_FILE="$toolchainPath" `
        -D CMAKE_MAKE_PROGRAM="$ninjaPath" `
        -G Ninja `
        ..

    if ($LASTEXITCODE -ne 0) {
        throw "CMake configure failed, exit code: $LASTEXITCODE"
    }

    & $ninjaPath
    if ($LASTEXITCODE -ne 0) {
        throw "Ninja build failed, exit code: $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}
