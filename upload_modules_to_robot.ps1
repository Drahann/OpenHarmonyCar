param(
    [string]$ConfigPath = (Join-Path $PSScriptRoot "config.txt"),
    [string[]]$Robot,
    [string]$RemoteDir = "/data/test",
    [switch]$Build
)

$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path

function Invoke-Step {
    param(
        [string]$Name,
        [scriptblock]$Action
    )

    Write-Host "==> $Name"
    & $Action
    if ($LASTEXITCODE -ne 0) {
        throw "$Name failed, exit code: $LASTEXITCODE"
    }
}

function Resolve-Robots {
    if ($Robot -and $Robot.Count -gt 0) {
        return $Robot
    }

    if (-not (Test-Path -LiteralPath $ConfigPath)) {
        throw "Config file not found: $ConfigPath"
    }

    $items = Get-Content -LiteralPath $ConfigPath |
        ForEach-Object { $_.Trim() } |
        Where-Object { $_ -and -not $_.StartsWith("#") }

    if (-not $items -or $items.Count -eq 0) {
        throw "No robot IP found in config: $ConfigPath"
    }

    return $items
}

function Get-DeployFiles {
    $files = @(
        @{ Name = "lidar_driver"; Path = Join-Path $Root "Lidar\build\lidar_driver" },
        @{ Name = "navigation";   Path = Join-Path $Root "Navi\build\navigation" },
        @{ Name = "serial";       Path = Join-Path $Root "NewWheelCtrl\bin\serial" },
        @{ Name = "udp2lcm";      Path = Join-Path $Root "NewWheelCtrl\bin\udp2lcm" }
    )

    foreach ($file in $files) {
        if (-not (Test-Path -LiteralPath $file.Path -PathType Leaf)) {
            throw "Deploy artifact not found: $($file.Path). Run this script with -Build or compile the module first."
        }
    }

    return $files
}

function Build-Modules {
    $ninja = "D:\Project\Harmony\ohos-sdk-5.0\windows\native\build-tools\cmake\bin\ninja.exe"

    if (Test-Path -LiteralPath (Join-Path $Root "Lidar\build\Makefile")) {
        Invoke-Step "Build Lidar" {
            cmake --build (Join-Path $Root "Lidar\build")
        }
    }

    if (Test-Path -LiteralPath $ninja) {
        Invoke-Step "Build Navi" {
            & $ninja -C (Join-Path $Root "Navi\build")
        }
        Invoke-Step "Build NewWheelCtrl" {
            & $ninja -C (Join-Path $Root "NewWheelCtrl\build")
        }
    }
    else {
        Invoke-Step "Build Navi" {
            cmake --build (Join-Path $Root "Navi\build")
        }
        Invoke-Step "Build NewWheelCtrl" {
            cmake --build (Join-Path $Root "NewWheelCtrl\build")
        }
    }
}

function Connect-Robot {
    param([string]$Target)

    Invoke-Step "Connect $Target" {
        hdc tconn $Target
    }
}

function Prepare-Remote {
    param([string]$Target)

    Invoke-Step "Prepare ${Target}:$RemoteDir" {
        hdc -t $Target shell mount -o remount -rw /
        hdc -t $Target shell "mkdir -p $RemoteDir"
    }
}

function Send-FileToRobot {
    param(
        [string]$Target,
        [string]$LocalPath
    )

    Invoke-Step "Send $(Split-Path -Leaf $LocalPath) to $Target" {
        hdc -t $Target file send $LocalPath "$RemoteDir/"
    }
}

function Chmod-RemoteFiles {
    param(
        [string]$Target,
        [object[]]$Files
    )

    $names = ($Files | ForEach-Object { "$RemoteDir/$($_.Name)" }) -join " "
    Invoke-Step "Chmod deploy files on $Target" {
        hdc -t $Target shell "chmod 777 $names"
    }
}

if ($Build) {
    Build-Modules
}

$robots = Resolve-Robots
$deployFiles = Get-DeployFiles

foreach ($ip in $robots) {
    $target = if ($ip.Contains(":")) { $ip } else { "${ip}:8710" }
    Connect-Robot $target
    Prepare-Remote $target

    foreach ($file in $deployFiles) {
        Send-FileToRobot $target $file.Path
    }

    Chmod-RemoteFiles $target $deployFiles
    Write-Host "Done: $target"
}
