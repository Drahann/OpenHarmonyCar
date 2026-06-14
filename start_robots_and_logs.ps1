param(
    [string]$ConfigPath = (Join-Path $PSScriptRoot "config.txt"),
    [string]$RemoteDir = "/data/test",
    [string]$LogRoot = (Join-Path $PSScriptRoot "robot_logs"),
    [string[]]$LogNames = @("navi.log","serial.log","udp2lcm.log"),
    [int]$IntervalMs = 500,
    [int]$ReconnectDelaySeconds = 2,
    [switch]$NoStart,
    [switch]$NoViewer
)

$ErrorActionPreference = "Stop"

function Get-Robots {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "Config file not found: $Path"
    }

    $items = Get-Content -LiteralPath $Path |
        ForEach-Object { $_.Trim() } |
        Where-Object { $_ -and -not $_.StartsWith("#") }

    if (-not $items -or $items.Count -eq 0) {
        throw "No robot IP found in config: $Path"
    }

    return $items
}

function Get-Target {
    param([string]$Robot)

    if ($Robot.Contains(":")) {
        return $Robot
    }

    return "${Robot}:8710"
}

function Get-SafeName {
    param([string]$Robot)

    return ($Robot -replace '[:\\/\*\?"<>\|]', "_")
}

function Invoke-Hdc {
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

function Connect-HdcTarget {
    param(
        [string]$Hdc,
        [string]$Target,
        [int]$RetryDelaySeconds
    )

    while ($true) {
        Write-Host "==> Connect $Target"
        & $Hdc tconn $Target
        if ($LASTEXITCODE -eq 0) {
            return
        }

        Write-Warning "Connect $Target failed, retrying in $RetryDelaySeconds seconds..."
        Start-Sleep -Seconds $RetryDelaySeconds
    }
}

$hdc = (Get-Command hdc -ErrorAction Stop).Source
$python = (Get-Command python -ErrorAction Stop).Source
$viewer = Join-Path $PSScriptRoot "watch_robot_logs.py"

if (-not (Test-Path -LiteralPath $viewer)) {
    throw "Viewer script not found: $viewer"
}

$robots = @(Get-Robots $ConfigPath)
if ($robots.Count -lt 2) {
    Write-Warning "Only $($robots.Count) robot IP found. Add the second robot IP to config.txt for dual-robot logging."
}

New-Item -ItemType Directory -Force -Path $LogRoot | Out-Null
foreach ($robot in $robots) {
    New-Item -ItemType Directory -Force -Path (Join-Path $LogRoot (Get-SafeName $robot)) | Out-Null
}

if (-not $NoStart) {
    foreach ($robot in $robots) {
        $target = Get-Target $robot
        Connect-HdcTarget -Hdc $hdc -Target $target -RetryDelaySeconds $ReconnectDelaySeconds
        Invoke-Hdc "Start robot $target" {
            & $hdc -t $target shell "$RemoteDir/test.sh" run
        }
    }
}

$jobs = @()
try {
    foreach ($robot in $robots) {
        $target = Get-Target $robot
        $safeName = Get-SafeName $robot
        $localDir = Join-Path $LogRoot $safeName

        $jobs += Start-Job -Name "logs-$safeName" -ScriptBlock {
            param($Hdc, $Target, $RemoteDir, $LocalDir, $LogNames, $IntervalMs, $ReconnectDelaySeconds)

            function Connect-HdcTarget {
                param(
                    [string]$Hdc,
                    [string]$Target,
                    [int]$RetryDelaySeconds
                )

                while ($true) {
                    & $Hdc tconn $Target | Out-Null
                    if ($LASTEXITCODE -eq 0) {
                        return
                    }

                    Write-Warning "Connect $Target failed, retrying in $RetryDelaySeconds seconds..."
                    Start-Sleep -Seconds $RetryDelaySeconds
                }
            }

            while ($true) {
                Connect-HdcTarget -Hdc $Hdc -Target $Target -RetryDelaySeconds $ReconnectDelaySeconds
                foreach ($logName in $LogNames) {
                    & $Hdc -t $Target file recv "$RemoteDir/$logName" "$LocalDir\" | Out-Null
                }
                Start-Sleep -Milliseconds $IntervalMs
            }
        } -ArgumentList $hdc, $target, $RemoteDir, $localDir, $LogNames, $IntervalMs, $ReconnectDelaySeconds

        Write-Host "Started log receiver for $target -> $localDir"
    }

    if ($NoViewer) {
        Write-Host "Log receivers are running. Press Ctrl+C to stop."
        while ($true) {
            Start-Sleep -Seconds 1
        }
    }
    else {
        & $python $viewer --config $ConfigPath --log-root $LogRoot --logs $LogNames
    }
}
finally {
    foreach ($job in $jobs) {
        Stop-Job -Job $job -ErrorAction SilentlyContinue
        Remove-Job -Job $job -Force -ErrorAction SilentlyContinue
    }
}
