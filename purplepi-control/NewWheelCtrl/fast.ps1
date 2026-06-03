Remove-Item -Path .\build -Recurse -Force
try {
    .\compile.ps1 -STL shared -Arch v8
}
catch {
    exit 1
}
Copy-Item .\bin\udp2lcm D:\Fast\module\test_mod\ -Force