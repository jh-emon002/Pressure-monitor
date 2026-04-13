# Dual-Channel Pressure Monitoring System

A complete embedded system for monitoring pressure from two independent sensors with configurable thresholds, persistent settings, and timestamped event logging via WiFi.

## Project Overview

This project implements a dual-channel pressure monitoring device using ESP32 microcontroller. The system provides local control through LCD and buttons while enabling remote monitoring and configuration through a web-based dashboard.

## Features

- Dual-channel pressure sensor monitoring (0-200 kPa range)
- Switchable display units (kPa / mmHg)
- Independent threshold configuration per channel
- Persistent storage of all user settings (survives power cycles)
- Dedicated digital alarm outputs (LED + buzzer)
- Timestamped event logging with NTP synchronization
- Web-based dashboard for remote monitoring
- CSV export of event logs

## Hardware Requirements

### Components

- ESP32 Development Board
- 2x Analog Pressure Sensors
- 16x2 I2C LCD Display
- 3x Push Buttons
- 2x LEDs
- 1x Buzzer
- Resistors and connecting wires

### Pin Configuration

| Component | ESP32 GPIO | Description |
|-----------|------------|-------------|
| Sensor 1 | GPIO 34 | Analog input for channel 1 |
| Sensor 2 | GPIO 35 | Analog input for channel 2 |
| Button 1 | GPIO 14 | Mode switch |
| Button 2 | GPIO 26 | Channel switch / Threshold increase |
| Button 3 | GPIO 27 | Unit toggle / Threshold decrease |
| LED 1 | GPIO 18 | Channel 1 alarm indicator |
| LED 2 | GPIO 19 | Channel 2 alarm indicator |
| Buzzer | GPIO 23 | Audio alarm output |
| LCD SDA | GPIO 21 | I2C data line |
| LCD SCL | GPIO 22 | I2C clock line |

## Software Requirements

### Arduino IDE Setup

1. Install Arduino IDE (version 2.x.x)
2. Add ESP32 board support:
   - Open File > Preferences
   - Add to Additional Board Manager URLs: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Open Tools > Board > Boards Manager
   - Search for "esp32" and install

### Required Libraries

Install through Arduino Library Manager:

- ArduinoJson
- LiquidCrystal_I2C

## Installation and Setup

### 1. WiFi Configuration

Edit the following lines in `src/final_code.ino`:

```cpp
const char* ssid = "Utonium";
const char* password = "verticalline";
```
put your wifi SSID and password here

### 2. Upload Firmware

1. Connect ESP32 to computer via USB
2. Open `src/final_code.ino` in Arduino IDE
3. Select correct board: Tools > Board > ESP32 Dev Module
4. Select correct port: Tools > Port > (your ESP32 port)
5. Click Upload

### 3. Monitor LCD Display

1. An IP address will be displayed for 3 seconds at startup, note it down
2. Current pressure reading, current channel and mode will be displayed

### 4. Access Web Dashboard

1. Open `dashboard.html` in web browser
2. Enter the ESP32 IP address that you noted in the connection field
3. Click "Connect"

The dashboard can be accessed from any device on the same WiFi network.

## Operation Guide

### Button Controls
Buttons are named as follows-
from left - first | second | third

The system operates in two modes:

**Normal Mode (NRM)**
- first button: Enter Threshold setting mode
- second button: Switch between Channel 1 and Channel 2
- third button: Toggle display unit (kPa / mmHg)

**Threshold Mode (THR)**
- first button: Return to Normal mode
- second button: Increase threshold for active channel
- third button: Decrease threshold for active channel

### LCD Display Format

```
CH1: 125.5kPa
TH:100.0kPa  NRM
```

Line 1: Active channel number, current pressure reading, unit
Line 2: Threshold value for active channel, current mode

### Alarm Behavior

When a channel's pressure exceeds its configured threshold:
- Corresponding LED illuminates
- Buzzer sounds
- Timestamped event logged to CSV file
- Web dashboard shows alarm status

Alarm clears when pressure drops below threshold.

### Web Dashboard Features

- Real-time pressure monitoring (1 Hz update rate)
- Current threshold display (read-only, auto-updates)
- Threshold configuration (separate input fields per channel)
- Unit selection (kPa / mmHg)
- Event log viewer with color coding:
  - Red: Threshold crossed
  - Green: Returned to normal
- CSV download button for event history

## Event Logging

Events are logged when channels cross thresholds or return to normal:

**CSV Format:**
```
Timestamp,Event
2024-04-11 15:23:45,CH1 crossed threshold
2024-04-11 15:25:12,CH1 returned to normal
```

## Persistent Storage

The following settings are saved to non-volatile storage:
- Channel 1 threshold
- Channel 2 threshold
- Active channel selection
- Display unit preference

These settings survive complete power loss and are restored on startup.


## Technical Specifications

- Microcontroller: ESP32 dev nodule
- ADC Resolution: 12-bit (0-4095)
- Pressure Range: 0-200 kPa (0-1500 mmHg)
- Display: 16x2 LCD (I2C)


## License

This project is submitted as part of FSMB EE Recruitment FEB 2026 Phase 2 Technical Assessment.

## Author

Md Jaied Hasan\
Dept of EEE, BUET\
mdjaiedhasan02@gmail.com\
01722656638

