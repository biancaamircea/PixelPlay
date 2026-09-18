param(
    [string]$Port = "COM4",
    [int]$BaudRate = 921600,
    [switch]$TestShot,
    [switch]$TestVideo,
    [switch]$OneShot
)

$ErrorActionPreference = "Stop"

$photosDir = Join-Path $PSScriptRoot "photos"
$videosDir = Join-Path $PSScriptRoot "videos"
New-Item -ItemType Directory -Force -Path $photosDir | Out-Null
New-Item -ItemType Directory -Force -Path $videosDir | Out-Null

$videoStream = $null
$videoFolder = $null
$videoMjpegPath = $null
$videoMp4Path = $null
$videoFrame = 0

$repoRoot = Split-Path -Parent $PSScriptRoot
$ffmpegPath = Join-Path $repoRoot "tools\ffmpeg\ffmpeg-9.0.1-essentials_build\bin\ffmpeg.exe"

$serial = [System.IO.Ports.SerialPort]::new($Port, $BaudRate, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
$serial.ReadTimeout = 500
$serial.WriteTimeout = 500
$serial.DtrEnable = $false
$serial.RtsEnable = $false
$serial.Open()

try {
    Start-Sleep -Milliseconds 300
    $serial.RtsEnable = $true
    Start-Sleep -Milliseconds 200
    $serial.RtsEnable = $false
    Start-Sleep -Seconds 2

    Write-Host "Listening on $Port at $BaudRate baud"
    Write-Host "Photos folder: $photosDir"
    Write-Host "Videos folder: $videosDir"
    Write-Host "Leave this window open, then choose FA POZA or FILMEAZA on the display."

    if ($TestShot) {
        $serial.Write("p")
        Write-Host "Sent test photo command."
    }

    if ($TestVideo) {
        $serial.Write("v")
        Write-Host "Sent test video command."
    }

    $line = ""
    while ($true) {
        try {
            $byte = $serial.BaseStream.ReadByte()
        }
        catch [TimeoutException] {
            continue
        }

        if ($byte -lt 0) {
            continue
        }

        $char = [char]$byte
        if ($char -eq "`n") {
            $text = $line.Trim()
            $line = ""

            if ($text -match '^(READY|CAPTURE_|PHOTO_|CAMERA_|VIDEO_|FRAME_)') {
                Write-Host $text
            }

            if ($text -match '^VIDEO_BEGIN\s+(\d+)$') {
                if ($null -ne $videoStream) {
                    $videoStream.Close()
                    $videoStream = $null
                }

                $timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
                $videoFolder = Join-Path $videosDir "video_$timestamp"
                New-Item -ItemType Directory -Force -Path $videoFolder | Out-Null
                $videoMjpegPath = Join-Path $videosDir "video_$timestamp.mjpeg"
                $videoMp4Path = Join-Path $videosDir "video_$timestamp.mp4"
                $videoStream = [System.IO.File]::Open($videoMjpegPath, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write)
                $videoFrame = 0
                Write-Host "Saving video frames to $videoFolder"
                Write-Host "Saving MJPEG to $videoMjpegPath"
                Write-Host "MP4 will be saved to $videoMp4Path"
            }

            if ($text -match '^PHOTO_BEGIN\s+(\d+)$') {
                $length = [int]$Matches[1]
                $buffer = [byte[]]::new($length)
                $offset = 0

                while ($offset -lt $length) {
                    $read = $serial.BaseStream.Read($buffer, $offset, $length - $offset)
                    if ($read -gt 0) {
                        $offset += $read
                    }
                }

                $timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
                $path = Join-Path $photosDir "photo_$timestamp.jpg"
                [System.IO.File]::WriteAllBytes($path, $buffer)
                Write-Host "Saved $path"

                if ($OneShot) {
                    break
                }
            }

            if ($text -match '^FRAME_BEGIN\s+(\d+)$') {
                $length = [int]$Matches[1]
                $buffer = [byte[]]::new($length)
                $offset = 0

                while ($offset -lt $length) {
                    $read = $serial.BaseStream.Read($buffer, $offset, $length - $offset)
                    if ($read -gt 0) {
                        $offset += $read
                    }
                }

                if ($null -eq $videoStream) {
                    $timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
                    $videoFolder = Join-Path $videosDir "video_$timestamp"
                    New-Item -ItemType Directory -Force -Path $videoFolder | Out-Null
                    $videoMjpegPath = Join-Path $videosDir "video_$timestamp.mjpeg"
                    $videoMp4Path = Join-Path $videosDir "video_$timestamp.mp4"
                    $videoStream = [System.IO.File]::Open($videoMjpegPath, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write)
                    $videoFrame = 0
                    Write-Host "Saving video frames to $videoFolder"
                    Write-Host "Saving MJPEG to $videoMjpegPath"
                    Write-Host "MP4 will be saved to $videoMp4Path"
                }

                $videoFrame++
                $framePath = Join-Path $videoFolder ("frame_{0:D4}.jpg" -f $videoFrame)
                [System.IO.File]::WriteAllBytes($framePath, $buffer)
                $videoStream.Write($buffer, 0, $buffer.Length)
                Write-Host "Saved frame $videoFrame"
            }

            if ($text -match '^VIDEO_END\s+(\d+)$') {
                if ($null -ne $videoStream) {
                    $videoStream.Close()
                    $videoStream = $null
                }
                Write-Host "Video saved with $($Matches[1]) frames."
                if ((Test-Path $ffmpegPath) -and ($null -ne $videoFolder) -and ($null -ne $videoMp4Path)) {
                    Write-Host "Converting to MP4..."
                    $framePattern = Join-Path $videoFolder "frame_%04d.jpg"
                    & $ffmpegPath -y -framerate 6 -i $framePattern -c:v libx264 -pix_fmt yuv420p $videoMp4Path | Out-Null
                    if ($LASTEXITCODE -eq 0) {
                        Write-Host "Saved MP4 $videoMp4Path"
                    }
                    else {
                        Write-Host "MP4 conversion failed. MJPEG and frames were still saved."
                    }
                }
                else {
                    Write-Host "FFmpeg not found, so MP4 was not created. MJPEG and frames were saved."
                }
            }
        }
        elseif ($char -ne "`r") {
            $line += $char
        }
    }
}
finally {
    if ($null -ne $videoStream) {
        $videoStream.Close()
    }
    if ($serial.IsOpen) {
        $serial.Close()
    }
}
