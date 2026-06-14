Remove-Item -Path .\build -Recurse -Force
try {
    .\compile.ps1 -STL shared -Arch v8
}
catch {
    exit 1
}
