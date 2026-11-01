# 🐾 CottonKitty / Little Reminder — ESP32-C3 OLED Animated Companion

> A cute, portable, battery-powered desk pet and emotional companion widget built on the **ESP32-C3 SuperMini** with a **0.96" SSD1306 OLED display**, tactile button menu, capacitive touch interactions, persistent Flash NVS memory, and dual **USB-C OTG Web Serial & Wi-Fi** control.

[![Vercel Live App](https://img.shields.io/badge/Web%20App-cotton--kitty.vercel.app-000000?style=for-the-badge&logo=vercel)](https://cotton-kitty.vercel.app)
[![Hardware](https://img.shields.io/badge/Hardware-ESP32--C3%20SuperMini-E7352C?style=for-the-badge&logo=espressif)](https://espressif.com)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)](https://opensource.org/licenses/MIT)

---

## 🌐 Live Web Companion

Connect and control your companion right from your browser (via USB-C OTG cable or Wi-Fi):  
👉 **[https://cotton-kitty.vercel.app](https://cotton-kitty.vercel.app)**

---

## ✨ Features

- 🐈 **9 Gallery Animations & Original Drawings**:
  - **Heart-Eyes Face** (28-frame smooth animation)
  - **Sad Face** (10-frame expressive animation)
  - **Walking Cat** (28-frame walking kitty animation)
  - **Cozy Cat** 🐾 (Resting cat drawing)
  - **Cat & Mouse Cuddle** (Drawing)
  - **Dove with Flower** (Drawing)
  - **Girl with Umbrella** (Drawing)
  - **Couple High-Five** (Drawing)
  - **Girl Portrait** (Drawing)
- 🐱 **32 Expressive Kaomoji Faces**:
  - `happy`, `love`, `shy`, `sleep`, `laugh`, `cry`, `angry`, `surprise`, `wink`, `cool`, `sad`, `blush`, `excited`, `dance`, `thinking`, `confused`, `ghost`, `bunny`, `star`, `heart`, `sleepy`, `proud`, `oops`, `relaxed`, `sparkle`, `tiny` (kitty ears!), `robot`, `kiss`, `pout`, `party`, `floating`, `calm`.
  - Natural blinking, dancing head-bob, and floating sparkle animations.
- 🎨 **Add New Faces from [oledanimationmaker.com](https://www.oledanimationmaker.com/)**:
  - Paste `.ino` code directly into the Web App.
  - Real-time 128×64 pixel canvas preview with automatic polarity detection.
  - 1-click upload over USB-C OTG or Wi-Fi straight to the OLED display!
- 📋 **On-Screen OLED Tactile Menu System**:
  - Open the menu anytime using **Button 2 (GPIO 1)**.
  - Scroll through options using **Button 1 (GPIO 9)**.
  - Sub-menus: `1. Faces (original)`, `2. Expressions`, `3. Keep as Default`, `4. Preset Messages`, `5. Settings & Power`.
- 💾 **Persistent Flash NVS Memory (`Preferences.h`)**:
  - Remembers your chosen **Default Boot Face** and any **Custom Uploaded Faces** across reboots and battery power switches!
- 👆 **Interactive Touch Gestures (TTP223 on GPIO 3)**:
  - **Single Tap**: Love heart reaction with floating hearts.
  - **Double Tap**: **Heart Explosion Burst!**
  - **Long Press (>1.5s)**: Cozy purr snooze with floating `Z z z`.
- 🔋 **Battery Power & Auto-Sleep Saver**:
  - Automatically shuts off the OLED screen after 45s of inactivity.
  - **Instant wake** on touching its head or pressing any button!
- ⚡ **Dual Control Modes**:
  - **USB-C OTG Cable**: Direct connection to Android phones, Mac, or PC with zero Wi-Fi needed (Web Serial API at 115200 baud).
  - **Wi-Fi Web Server**: Connects to home network (`ESP` / `Ajay@esp`) with automatic AP hotspot fallback (`CottonKitty` / `kittyfriend` at `http://192.168.4.1`).

---

## 🔌 Hardware Components & Bill of Materials (BOM)

| Component | Specification | Description |
| :--- | :--- | :--- |
| **Microcontroller** | ESP32-C3 SuperMini | 160MHz RISC-V, built-in Wi-Fi & BLE, USB-C |
| **Display** | 0.96" SSD1306 OLED | 128×64 Monochrome I2C Display (0x3C) |
| **Touch Sensor** | TTP223 Capacitive Module | Digital touch output (Active HIGH) |
| **Tactile Buttons** | 6×6×5mm Tactile Push Buttons (2x) | Button 1 (Next/Down) & Button 2 (Menu/Select) |
| **Battery** | 3.7V LiPo / Li-Ion (500mAh – 1000mAh) | Provides hours of portable battery life |
| **Charging Module** | TP4056 with Protection (Type-C) | Safe Li-Ion charging and over-discharge cutoff |
| **Power Switch** | SPDT Micro Slide Switch | Power ON/OFF toggle between battery and circuit |

---

## 📐 Wiring & Pinout Diagram

### Pin Connections Table

| Component | Pin on Module | ESP32-C3 SuperMini Pin | Note |
| :--- | :--- | :--- | :--- |
| **0.96" OLED (SSD1306)** | `VCC` | `3V3` | 3.3V Power |
| | `GND` | `GND` | Common Ground |
| | **`SDA`** | **`GPIO 5`** | Custom I2C Data line |
| | **`SCL`** | **`GPIO 6`** | Custom I2C Clock line |
| **Touch Sensor (TTP223)** | `VCC` | `3V3` | Power |
| | `GND` | `GND` | Common Ground |
| | **`OUT`** | **`GPIO 3`** | Active HIGH digital output |
| **Button 1 (Next / Scroll)** | Pin 1 / Pin 2 | **`GPIO 9`** to `GND` | Active LOW (`INPUT_PULLUP`) |
| **Button 2 (Menu / Select)** | Pin 1 / Pin 2 | **`GPIO 1`** to `GND` | Active LOW (`INPUT_PULLUP`) |

### Battery & Charging Circuit

```
[3.7V LiPo Battery]
   (+) ──> TP4056 (B+)
   (-) ──> TP4056 (B-)

[TP4056 Charger Output]
   (OUT-) ────────────────────────────> Common GND
   (OUT+) ──> [Slide Switch] ──> ESP32-C3 (5V Pin)
```

---

## 🕹️ Button & Touch Navigation Guide

```
             ┌─────────────────────────┐
             │       0.96" OLED        │
             │         128x64          │
             └─────────────────────────┘
              [BTN 1: GP9]  [BTN 2: GP1]
               Next / Down   Menu / Select
```

| Control | In Normal Face Display | In OLED Menu |
| :--- | :--- | :--- |
| **Button 1 (GPIO 9)** | Short press: **Cycle next face** (41 modes) | **Scroll down** through menu items |
| **Button 2 (GPIO 1)** | Short press: **Open OLED Menu** | **Select / Enter** sub-menu / Toggle |
| **Touch (GPIO 3)** | Single Tap: Love Heart Reaction<br>Double Tap: **Heart Explosion Burst**<br>Long Press (>1.5s): Snooze Zzz | Wakes device if sleeping |

---

## 🛠️ Software Setup & Flashing

### Requirements
- **Arduino IDE** (or `arduino-cli`)
- ESP32 Board Package (by Espressif)
- Libraries:
  - `Adafruit SSD1306`
  - `Adafruit GFX Library`
  - `Preferences` (built-in with ESP32 core)

### Arduino IDE Settings
- **Board**: `ESP32C3 Dev Module`
- **CPU Frequency**: `160MHz`
- **Flash Size**: `4MB`
- **Partition Scheme**: `Default 4MB with spiffs (1.2MB APP / 1.5MB SPIFFS)`
- **Upload Speed**: `921600` (or `115200`)
- **USB CDC On Boot**: `Enabled`

### Flash via Command Line (Mac / Linux):
```bash
arduino-cli compile --fqbn esp32:esp32:esp32c3 LittleReminder.ino
arduino-cli upload -p /dev/cu.usbmodem1101 --fqbn esp32:esp32:esp32c3 LittleReminder.ino
```

---

## 📦 Custom Device Orders & Inquiries

Want a fully assembled, custom 3D-printed desktop companion or custom animated hardware device?

- 📧 **Order & Inquiries Email**: [ajaykumarjaganathan@gmail.com](mailto:ajaykumarjaganathan@gmail.com)
- 🌐 **Portfolio & Projects**: [https://ajforge-dev.github.io/](https://ajforge-dev.github.io/)

---

## 📄 License
This project is open-source under the [MIT License](LICENSE).
