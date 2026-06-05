Remove-Item -Path .\build -Recurse -Force
try {
    .\compile.ps1 -STL shared -Arch v8
}
catch {
    exit 1
}

Copy-Item .\build\navigation D:\Fast\module\test_mod\ -Force
cd D:\Fast\module\