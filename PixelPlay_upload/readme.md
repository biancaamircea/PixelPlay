# PixelPlay

PixelPlay is a mini-arcade project built on the FRDM-MCXA153 board. It includes a TFT display menu, user selection, two games, and ESP32-CAM support for taking photos and recording short videos saved to a laptop.

## Features

- TFT display menu
- User selection: Anastasia / Bianca
- Flappy Bird game
- Dino Run game
- Photo capture with ESP32-CAM
- 5-second video recording with ESP32-CAM
- Onboard and external button support

## Hardware

- FRDM-MCXA153
- TFT display
- ESP32-CAM-MB
- External push buttons
- Jumper wires

## Camera Wiring

```text
ESP32 IO14 -> FRDM J5[3]  / P3_14
ESP32 IO13 -> FRDM J4[2]  / P1_10
ESP32 GND  -> FRDM J3[12] / GND
```

Power both boards separately through USB. Do not connect 5V or 3.3V between the boards.

## Photo/Video Server

Run this PowerShell command and leave the window open before choosing `FA POZA` or `FILMEAZA` on the display:

```powershell
powershell -ExecutionPolicy Bypass -File "esp32_cam\read_serial_photo.ps1" -Port COM4
```

If the ESP32-CAM appears on a different COM port, replace `COM4` with the detected port.
