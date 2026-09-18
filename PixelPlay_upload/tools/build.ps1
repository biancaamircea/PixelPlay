Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$ninja = "C:\Users\mirce\.mcuxpressotools\ninja-1.13.2\ninja.exe"

Push-Location $projectRoot
try {
    cmake --preset debug
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    Push-Location "$projectRoot\debug"
    try {
        $commands = & $ninja -C . -t commands new.elf
        foreach ($command in $commands) {
            if ([string]::IsNullOrWhiteSpace($command)) {
                continue
            }

            cmd.exe /C $command
            if ($LASTEXITCODE -ne 0) {
                exit $LASTEXITCODE
            }
        }
    }
    finally {
        Pop-Location
    }
}
finally {
    Pop-Location
}
