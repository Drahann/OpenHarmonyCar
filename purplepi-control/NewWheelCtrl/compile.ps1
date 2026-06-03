param (
    [ValidateSet("shared","static")] 
    [string]$STL,

    [ValidateSet("v8","v7")]
    [string]$Arch
)
#在此处设置你的native所在路径
$native_path = "D:\Project\Harmony\ohos-sdk-5.0\windows\native"

Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path build | Out-Null
Set-Location build
$ninja_path = ${native_path} + "\build-tools\cmake\bin\ninja.exe"

if($STL -eq "shared"){
    $C_STL="c++_shared"
}
elseif($STL -eq "static"){
    $C_STL="c++_static"
}
else {}
if($Arch -eq "v8"){
    $C_Arch="arm64-v8a"
}
elseif($Arch -eq "v7"){
    $C_Arch="armeabi-v7a"
}
else{}

# 注意这里是win11的powershell环境, 添加临时环境变量

$toolchain_dir=${native_path}+"\build\cmake"
#Write-Output "${native_path}"
#Write-Output "${ninja_path}"
#Write-Output "$toolchain_dir "
if ($env:Path -notlike "*${toolchain_dir}*") {
    $env:Path+=";${toolchain_dir}"
     
}
else{}
if ($env:Path -notlike "*${ninja_path}*") {
     $env:Path+=";${toolchain_dir}"
     
}
else{}

#Write-Output "$STL : $C_STL , $Arch : $C_Arch"

cmake  `
     -D CMAKE_C_COMPILER="${native_path}\llvm\bin\clang.exe" `
     -D CMAKE_CXX_COMPILER="${native_path}\llvm\bin\clang++.exe" `
     -D OHOS_STL="$C_STL" `
     -D OHOS_ARCH="$C_Arch" `
     -D OHOS_PLATFORM=OHOS `
     -D CMAKE_TOOLCHAIN_FILE="${native_path}\build\cmake\ohos.toolchain.cmake" .. `
     -D CMAKE_MAKE_PROGRAM="$ninja_path" `
     -G Ninja 

ninja
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake 失败，退出码: $LASTEXITCODE"
    exit $LASTEXITCODE
}
cd ..