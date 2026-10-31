# 🐾 Little Reminder — ESP32-C3 OLED Pocket Pet & Love Companion

A cute, portable, battery-powered desk pet and emotional companion widget built on the **ESP32-C3 SuperMini** with a **0.96\" SSD1306 OLED display**, capacitive touch interactions, on-screen menu, and mobile Wi-Fi control.

---

## ✨ Features

- 🐶 **10 Vector & Bitmap Faces**: Puppy, Cozy Fox, Kitty, Panda, Hamster, Bunny, Happy, Wink, Loving, Sleepy.
- 👆 **Touch Sensor Gestures**:
  - **Single Tap**: Triggers face-specific reaction animations (Puppy wags ears & barks hearts, Kitty purrs, Fox blinks, Panda blushes).
  - **Double Tap**: Massive radiant heart explosion with shooting stars (`<3 LOVE YOU <3`).
  - **Long Press (>1.4s)**: Cozy purr snooze with floating `Z z z`.
- 🌙 **Auto-Sleep Battery Saver**: Automatically turns off the OLED display after 45s idle; wakes up instantly when touching its head or pressing a button.
- 📋 **On-Screen OLED Settings Menu**:
  - Toggle **Auto-Sleep [ON/OFF]**
  - Toggle **Wi-Fi AP [ON/OFF]** on-demand (saving ~80mA of battery when off!)
  - Toggle **Double-Tap [ON/OFF]**
- 📱 **Embedded Mobile Web App**: Turn on Wi-Fi on the device, connect to `LittleReminder` from your phone, and open `http://192.168.4.1` to:
  - Switch faces with 1 tap
  - Send custom reminder notes to the OLED screen
  - Upload new `.ino` faces from *oledanimationmaker.com* with auto-polarity correction

---

## 🔌 Hardware Wiring

| Component | Pin on Component | ESP32-C3 SuperMini Pin | Note |
| :--- | :--- | :--- | :--- |
| **0.96\" OLED (SSD1306)** | `VCC` / `GND` | `3V3` / `GND` | 3.3V Power |
| | `SDA` | **`GPIO 5`** | Custom I2C Data |
| | `SCL` | **`GPIO 6`** | Custom I2C Clock |
| **Touch Sensor (TTP223)** | `VCC` / `GND` | `3V3` / `GND` | Active HIGH digital output |
| | `OUT` / `SIG` | **`GPIO 3`** | Touch input |
| **Button 1 (Face / Next)** | Leg 1 / Leg 2 | **`GPIO 9`** to `GND` | Active LOW (internal pullup) |
| **Button 2 (Menu / Select)**| Leg 1 / Leg 2 | **`GPIO 1`** to `GND` | Active LOW (internal pullup) |
| **Power (LiPo + TP4056)** | `TP4056 OUT+` | Slide Switch -> **`5V`** | Turn switch OFF when USB connected |
| | `TP4056 OUT-` | **`GND`** | Common Ground |

---

## 🛠️ Arduino IDE Setup

1. **Board**: `ESP32C3 Dev Module`
2. **USB CDC On Boot**: `Enabled`
3. **Libraries Required**:
   - `Adafruit SSD1306`
   - `Adafruit GFX Library`
4. Open `LittleReminder.ino` and click **Upload**.

---

## 📱 How to Connect with Mobile Phone

1. On the device, press **Button 2** (GPIO 1) to open the OLED menu.
2. Press **Button 1** (GPIO 9) to navigate to `4. Settings & Power`, then press **Button 2**.
3. On `Wi-Fi AP: [ OFF ]`, press **Button 2** to toggle it to **`[ ON ]`**.
4. On your mobile phone:
   - Connect to Wi-Fi: **`LittleReminder`**
   - Password: **`love1234`**
5. Open your mobile browser and go to:
   **`http://192.168.4.1`**
6. Tap any face to switch it immediately, or type a message and press **Display Message**!

---

## 📄 License
MIT License