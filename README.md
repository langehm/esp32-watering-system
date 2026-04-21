# 🪴 Smart ESP32 Watering System

A highly optimized, robust, and ultra-low-power automated watering system built for the ESP32 (specifically ESP32-S3). It utilizes FreeRTOS to perfectly separate UI rendering, hardware input, and watering logic.

## ✨ Key Features

* **Ultra-Low Power (Deep Sleep):** The system completely powers down between scheduled watering events or after 30 seconds of UI inactivity. It wakes up via a hardware interrupt (Menu button) or a precise RTC timer.
* **Smart Flash Storage (Wear-Leveling):** Uses a RAM-shadowing technique with a "dirty flag" (Write-Back Caching). Changes in the menu are only written to the flash memory (NVS) in a single block upon exiting the menu or before entering deep sleep, maximizing the flash lifespan.
* **Non-Blocking UI:** The OLED menu and the watering tasks run on separate FreeRTOS tasks. The UI never freezes, even when pumps are actively running.
* **Debounced Background Inputs:** Rotary encoder and button inputs are handled by a dedicated, high-speed (5ms) background task, guaranteeing zero missed inputs without blocking the main loop.
* **Clean Architecture:** Built using the "Ports and Adapters" pattern. Hardware-specific code is strictly encapsulated into namespaces (`RTCAdapter`, `NVSAdapter`, `PumpAdapter`, `DisplayAdapter`, `InputAdapter`) distributed across separate `.ino` files for maximum maintainability.

## 🧰 Hardware Requirements

* **Microcontroller:** ESP32-S3 (or standard ESP32 with adjusted pinout)
* **Display:** 128x64 OLED Display (SSD1306, I2C)
* **RTC Module:** DS3231 or DS1307 Real-Time Clock (I2C) with coin cell battery
* **Inputs:** 1x Rotary Encoder (with push button), 2x Push Buttons (Menu, Back)
* **Outputs:** 4-Channel Relay Module (Active-High) & 4x Water Pumps

### Pinout Configuration

| Component | Pin / GPIO | Notes |
| :--- | :--- | :--- |
| **I2C SDA** | `15` | Shared by OLED and RTC |
| **I2C SCL** | `16` | Shared by OLED and RTC |
| **Rotary CLK** | `46` | |
| **Rotary DT** | `3` | |
| **Rotary SW (Set)**| `8` | Button on encoder (Active LOW, internal Pull-Up) |
| **Menu Button** | `18` | Wake-up capable (Active HIGH, external Pull-Down) |
| **Back Button** | `17` | (Active HIGH, external Pull-Down) |
| **Pumps 1 - 4** | `10, 11, 12, 13`| Relay triggers |

*(Note: Pins can be easily modified in the respective Adapter files).*

## 🚀 How to Build and Flash

This project is structured into multiple `.ino` files (Tabs) to keep the codebase clean while avoiding complex C++ Header/CPP linking issues in the Arduino IDE.

### 1. Prerequisites
1. Install the **Arduino IDE** (v2.x recommended).
2. Install the **ESP32 Core** via the Boards Manager.
3. Install the following libraries via the Library Manager:
   * `Adafruit GFX Library`
   * `Adafruit SSD1306`
   * `RTClib` (by Adafruit)

### 2. Project Structure Setup
When cloning or downloading this repository, ensure that **all files are placed inside a folder named exactly like the main file** (`watering_system`).

Your folder structure must look exactly like this:
```text
watering_system/
├── watering_system.ino     # Main file (Tasks & Boss Logic)
├── Header.h                # Forward declarations for Adapters
├── display_service.ino     # OLED UI and drawing logic
├── input_service.ino       # Rotary & Button polling task
├── menu_service.ino        # State machine for the UI navigation
├── pump_service.ino        # Relay logic
├── rtc_service.ino         # DS3231 I2C logic
├── storage_service.ino     # NVS Flash logic (RAM cache)
└── watering_service.ino    # Pump execution task
```

### 3. Compilation and Flashing
1. Open `watering_system.ino` in the Arduino IDE. All other `.ino` files will automatically open as tabs.
2. Select your specific ESP32 board in the `Tools -> Board` menu (e.g., *ESP32S3 Dev Module*).
3. Connect your ESP32 via USB. Select the correct COM Port under `Tools -> Port`.
4. Click the **Upload** button (Right Arrow icon).
5. Open the Serial Monitor (`Tools -> Serial Monitor`) and set the baud rate to `115200` to verify the boot sequence and deep sleep triggers.

## 🎮 Usage Guide
* **Idle Screen:** Displays the current time, pump status, and a 4x24 matrix showing which pumps are scheduled for which hour.
* **Enter Menu:** Press the `Menu` button.
* **Navigation:** Turn the rotary encoder to scroll, press the encoder button to select (`Set`), and press the `Back` button to return.
* **Deep Sleep:** Leave the system alone for 30 seconds. It will save all unwritten changes to the flash memory, turn off the OLED, and enter ultra-low-power mode until the next scheduled watering time or until the `Menu` button is pressed.
