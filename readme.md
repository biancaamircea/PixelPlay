# PixelPlay

*A colorful FRDM-MCXA153 mini-arcade with games, user selection, and ESP32-CAM photo/video capture.*

## Description

PixelPlay is an embedded mini-arcade project built using the **NXP FRDM-MCXA153** development board. The project uses a TFT display to show a start screen, a two-user selection menu, a game menu, two playable games, and camera options for taking photos or recording short videos.

The interface is controlled with the onboard **SW2** and **SW3** buttons. External momentary buttons can also be connected in parallel with SW2 and SW3, so the same actions can be triggered from a breadboard. The project currently supports two users, **Anastasia** and **Bianca**, and both users access the same games and camera tools.

PixelPlay also integrates an **ESP32-CAM-MB** module. The FRDM board sends a trigger signal to the ESP32-CAM, while a PowerShell script running on the laptop listens over USB serial and saves received photos and videos into local folders.

## Motivation

The goal of PixelPlay was to combine several embedded systems concepts into one interactive project:

- display control using a TFT screen
- menu navigation with physical buttons
- simple real-time games
- communication between two boards
- camera capture using an external ESP32-CAM
- laptop-side file saving for photos and videos

Instead of building only a single game, the project was designed as a small arcade-style system with user selection and multiple features accessible from one menu.

## Features

- Welcome screen
- User selection for Anastasia and Bianca
- Game menu with two games
- Flappy Bird-style game
- Dino Run-style game
- Photo capture through ESP32-CAM
- 5-second video recording through ESP32-CAM
- Photos saved as JPG files on the laptop
- Videos saved as MJPEG frames and converted to MP4
- Onboard SW2/SW3 support
- External breadboard button support
- Camera status feedback on the display

## Architecture

The **FRDM-MCXA153** board is the main controller. It handles the display, menus, button input, game logic, and camera trigger signals.

The **ESP32-CAM-MB** handles camera capture. It is programmed separately and communicates with the laptop through USB serial. The FRDM board does not receive the image data directly; it only sends a hardware trigger to the ESP32-CAM.

The laptop runs a PowerShell script that listens to the ESP32-CAM serial output. When a photo or video is received, the script saves the file locally.

### System Components

| Component | Role |
|---|---|
| FRDM-MCXA153 | Main controller for menu, games, buttons, display, and camera trigger |
| TFT display | Shows welcome screen, user menu, games, and camera status |
| ESP32-CAM-MB | Captures photos and short videos |
| Laptop | Runs the serial photo/video receiver script |
| Onboard SW2/SW3 | Main user controls |
| External buttons | Optional parallel controls for SW2 and SW3 |
| Jumper wires | Connections between modules |

## Menu Flow

```text
Welcome screen
  -> User selection
      SW2: Anastasia
      SW3: Bianca
  -> Main menu
      FLAPPY BIRD
      DINO RUN
      FA POZA
      FILMEAZA
```

## Controls

| Screen / Mode | SW2 action | SW3 action |
|---|---|---|
| Welcome screen | Start | Start |
| User selection | Select Anastasia | Select Bianca |
| Game menu | Next option | Confirm option |
| Flappy Bird | Flap / restart | Back to menu |
| Dino Run | Jump / restart | Back to menu |
| Photo screen | Back to game menu | Back to user menu |
| Video screen | Back to game menu | Back to user menu |

External buttons connected to the SW2 and SW3 signals perform the same actions as the onboard buttons.

## Hardware

### Components Used

| Device | Usage |
|---|---|
| NXP FRDM-MCXA153 | Main embedded controller |
| TFT display | Graphical interface |
| ESP32-CAM-MB | Photo and video capture |
| External momentary buttons | Optional duplicated controls for SW2/SW3 |
| Jumper wires | Signal wiring |
| Laptop | Serial receiver for camera files |

## Wiring

### ESP32-CAM-MB to FRDM-MCXA153

Only three wires are connected between the ESP32-CAM-MB and the FRDM board:

```text
ESP32 IO14 -> FRDM J5[3]  / P3_14
ESP32 IO13 -> FRDM J4[2]  / P1_10
ESP32 GND  -> FRDM J3[12] / GND
```

Important:

- The ESP32-CAM-MB is powered separately through USB.
- The FRDM-MCXA153 is powered separately through USB.
- Do not connect 5V between the boards.
- Do not connect 3.3V between the boards.
- A common GND is required.

### External Buttons

The external buttons are momentary and active LOW:

- not pressed = HIGH
- pressed = LOW

SW2 external button:

```text
FRDM J4[11] / P3_29 -> SW2 external signal
FRDM J5[8]  / GND   -> SW2 external ground
```

SW3 external button:

```text
FRDM J1[1] / P1_7 -> SW3 external signal
FRDM J6[8] / GND  -> SW3 external ground
```

Breadboard placement used during testing:

```text
Button on g5/g6/g7 and i5/i6/i7:
  J4[11] / P3_29 -> f5
  J5[8]  / GND   -> f6

Button on g24/g25/g26 and i24/i25/i26:
  J1[1] / P1_7 -> f25
  J6[8] / GND  -> f26
```

## Software

| Part | Language / Tool | Role |
|---|---|---|
| FRDM firmware | C | Menu, games, display, buttons, camera trigger |
| ESP32-CAM firmware | Arduino / C++ | Camera capture and serial file transfer |
| Laptop receiver | PowerShell | Saves photos/videos from ESP32-CAM |
| Build system | CMake / MCUXpresso tools | Builds the FRDM firmware |

## Project Structure

```text
app/                         TFT display driver files
esp32_cam/                   ESP32-CAM sketch and laptop receiver script
frdmmcxa153/                 Board configuration files
tools/build.ps1              Build helper script
led_blinky.c                 Main PixelPlay firmware
readme.md                    Project documentation
```

Generated files are intentionally not uploaded:

```text
debug/
esp32_cam/photos/
esp32_cam/videos/
tools/ffmpeg/
tools/arduino-cli/
```

## Build

From PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File "C:\Users\mirce\Music\ipcei-lab-main\src\lab_new_project_configured\configured_flappy\tools\build.ps1"
```

The project can then be flashed/debugged from VS Code or MCUXpresso using the configured FRDM-MCXA153 debug setup.

## ESP32-CAM Upload

The ESP32-CAM firmware is located in:

```text
esp32_cam/esp32_cam_trigger_upload/esp32_cam_trigger_upload.ino
```

It uses:

- IO14 as trigger input from FRDM
- IO13 as done/status output to FRDM
- USB serial at 921600 baud for file transfer to the laptop

## Photo and Video Receiver

Before using `FA POZA` or `FILMEAZA` on the display, run:

```powershell
powershell -ExecutionPolicy Bypass -File "C:\Users\mirce\Music\ipcei-lab-main\src\lab_new_project_configured\configured_flappy\esp32_cam\read_serial_photo.ps1" -Port COM4
```

If the ESP32-CAM appears on another COM port, replace `COM4` with the correct port.

To check available COM ports:

```powershell
[System.IO.Ports.SerialPort]::GetPortNames()
```

Saved files:

```text
esp32_cam/photos/    JPG photos
esp32_cam/videos/    video frames, MJPEG files, and MP4 files
```

## Troubleshooting

### The camera command is received, but no photo is saved

If the terminal shows:

```text
CAPTURE_REQUESTED
CAPTURE_FAILED
```

then the FRDM trigger works, but the ESP32-CAM did not capture a frame. This is usually caused by camera module contact issues, USB power instability, or the ESP32-CAM resetting.

### COM4 does not exist

Check the available ports:

```powershell
[System.IO.Ports.SerialPort]::GetPortNames()
```

Use the COM port that appears for `USB-SERIAL CH340`.

### COM4 access denied

Close any other PowerShell window, Arduino Serial Monitor, or application using the ESP32-CAM serial port.

### External button blocks onboard SW2/SW3

Remove the external button wiring and test the onboard button again. If it works, the external button was probably shorting the signal to GND continuously.

## Implementation Notes

- The display and games run on the FRDM-MCXA153, not on Arduino.
- The ESP32-CAM is used only as a camera module and USB serial file sender.
- The FRDM board does not process or display the captured image.
- Videos are recorded for a fixed duration of 5 seconds.
- Audio is not recorded because the ESP32-CAM module used here does not include a microphone in this setup.

## Status

Current menu options:

- FLAPPY BIRD
- DINO RUN
- FA POZA
- FILMEAZA

The touch test and third game were removed from the final menu to keep the presentation version simpler and more stable.
