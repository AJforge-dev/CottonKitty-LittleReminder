/*
 * Little Reminder - ESP32-C3 SuperMini + 0.96\" OLED (SSD1306)
 *
 * FEATURES (Option B):
 * - 10 Vector & Bitmap Faces & Animations (Puppy, Fox, Kitty, Panda, Hamster, Bunny, etc.)
 * - Face-Specific Touch Reactions
 * - Double-Tap Touch Gesture (Heart Burst!)
 * - Long-Press Touch Gesture (Snooze / Purr Zzz)
 * - Auto-Sleep Battery Saver (with instant wake on touch/button)
 * - On-Screen OLED Menu with Settings:
 *     * Toggle Auto-Sleep [ON/OFF]
 *     * Toggle Wi-Fi Hotspot [ON/OFF] (Save 80mA battery!)
 *     * Toggle Double-Tap [ON/OFF]
 * - Wi-Fi AP Hotspot on demand (http://192.168.4.1)
 *
 * WIRING:
 * OLED VCC/GND -> 3V3/GND | SDA -> GPIO 5 | SCL -> GPIO 6
 * Touch Sensor -> VCC/GND -> 3V3/GND | OUT -> GPIO 3
 * Button 1 (Next Face / Menu Down)   -> GPIO 9 to GND
 * Button 2 (Menu Open / Select)       -> GPIO 1 to GND
 * Battery 3.7V -> TP4056 -> Slide Switch -> SuperMini 5V
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>

// ==========================================
// 📌 PIN CONFIGURATION (Change if needed)
// ==========================================
#define OLED_SDA       5   // Set to 5 (or 8 for standard I2C)
#define OLED_SCL       6   // Set to 6 (or 9 for standard I2C)
#define TOUCH_PIN      3   // Touch sensor OUT (Active HIGH)
#define BTN1_PIN       9   // Button 1: Next Face / Menu Down (Active LOW)
#define BTN2_PIN       1   // Button 2: Open Menu / Select (Active LOW)

#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define OLED_ADDR      0x3C

// ==========================================
// 📶 WI-FI NETWORK CONFIGURATION
// ==========================================
// 1. Device Hotspot (Default)
#define AP_SSID        "LittleReminder"
#define AP_PASS        "love1234"

// 2. Or connect to your Home Wi-Fi / Phone Hotspot:
// (Put your Wi-Fi name & password here if you want it to join your home network.
//  Leave empty "" to use the LittleReminder hotspot!)
const char* HOME_WIFI_SSID = "";   // e.g. "MyHomeWiFi"
const char* HOME_WIFI_PASS = "";   // e.g. "myPassword"

String wifiIP = "192.168.4.1";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WebServer server(80);

// ==========================================
// ⚙️ SETTINGS & STATE
// ==========================================
bool autoSleepEnabled = true;
uint32_t sleepTimeoutMs = 45000;  // 45 seconds idle -> sleep
bool wifiEnabled = true;          // ✅ AUTO-START WI-FI ON BOOT!
bool doubleTapEnabled = true;

bool isSleeping = false;
uint32_t lastActivityMillis = 0;

enum DisplayMode {
  MODE_FACE,
  MODE_REACTION,
  MODE_DOUBLE_TAP,
  MODE_MENU,
  MODE_MESSAGE
};

DisplayMode currentMode = MODE_FACE;
uint8_t currentFaceIndex = 0;
const uint8_t TOTAL_FACES = 10;
const char* const FACE_NAMES[TOTAL_FACES] = {
  "Puppy", "Cozy Fox", "Kitty", "Panda", "Hamster",
  "Bunny", "Happy", "Wink", "Loving", "Sleepy"
};

uint8_t customBitmap[1024];
bool hasCustomBitmap = false;

// Touch gesture state
uint32_t touchDownTime = 0;
uint32_t lastTapReleaseTime = 0;
bool touchWasPressed = false;
uint8_t tapCount = 0;

// Reaction animation state
uint32_t reactionStartMillis = 0;
uint32_t reactionDurationMs = 2600;
uint32_t animTick = 0;
uint32_t lastAnimTickMillis = 0;

// Menu system state
enum MenuLevel {
  MENU_MAIN,
  MENU_PETS,
  MENU_EXPR,
  MENU_MESSAGES,
  MENU_SETTINGS
};

MenuLevel currentMenuLevel = MENU_MAIN;
int8_t menuCursor = 0;
int8_t menuScroll = 0;

// Current custom message
String currentMessageText = "You are doing amazing! <3";

// Forward declarations
void wakeUp();
void goToSleep();
void showFace(uint8_t index);
void triggerReaction(bool isDoubleTap = false);
void renderCurrent();
void renderMenu();
void handleTouchGestures();
void handleButtons();
void updateWiFiState();

// ==========================================
// 🎨 DRAWING HELPERS
// ==========================================
void drawHeart(int16_t x, int16_t y, int16_t size, bool fill = true) {
  int16_t r = size / 4;
  if (r < 1) r = 1;
  if (fill) {
    display.fillCircle(x - r, y, r, SSD1306_WHITE);
    display.fillCircle(x + r, y, r, SSD1306_WHITE);
    display.fillTriangle(x - 2 * r, y, x + 2 * r, y, x, y + 2 * r + 1, SSD1306_WHITE);
  } else {
    display.drawCircle(x - r, y, r, SSD1306_WHITE);
    display.drawCircle(x + r, y, r, SSD1306_WHITE);
    display.drawLine(x - 2 * r, y, x, y + 2 * r + 1, SSD1306_WHITE);
    display.drawLine(x + 2 * r, y, x, y + 2 * r + 1, SSD1306_WHITE);
  }
}

void drawStar(int16_t x, int16_t y, int16_t r) {
  display.drawLine(x - r, y, x + r, y, SSD1306_WHITE);
  display.drawLine(x, y - r, x, y + r, SSD1306_WHITE);
  if (r > 3) {
    display.drawPixel(x - 1, y - 1, SSD1306_WHITE);
    display.drawPixel(x + 1, y - 1, SSD1306_WHITE);
    display.drawPixel(x - 1, y + 1, SSD1306_WHITE);
    display.drawPixel(x + 1, y + 1, SSD1306_WHITE);
  }
}

// Draw cute vector faces
void drawPuppy(uint32_t t = 0) {
  // Floppy ears
  display.fillRoundRect(22, 14, 14, 28, 6, SSD1306_WHITE);
  display.fillRoundRect(92, 14, 14, 28, 6, SSD1306_WHITE);
  display.fillRoundRect(25, 17, 8, 22, 4, SSD1306_BLACK);
  display.fillRoundRect(95, 17, 8, 22, 4, SSD1306_BLACK);

  // Head contour
  display.drawRoundRect(32, 12, 64, 46, 18, SSD1306_WHITE);

  // Big friendly puppy eyes
  display.fillCircle(48, 28, 7, SSD1306_WHITE);
  display.fillCircle(80, 28, 7, SSD1306_WHITE);
  display.fillCircle(46, 26, 2, SSD1306_BLACK); // pupil reflection
  display.fillCircle(78, 26, 2, SSD1306_BLACK);

  // Nose
  display.fillTriangle(64, 40, 58, 35, 70, 35, SSD1306_WHITE);

  // W-shaped cute mouth
  display.drawCircle(59, 44, 4, SSD1306_WHITE);
  display.drawCircle(69, 44, 4, SSD1306_WHITE);
  display.fillRect(52, 38, 24, 5, SSD1306_BLACK);
}

void drawFox(uint32_t t = 0) {
  // Pointy Fox Ears
  display.fillTriangle(26, 32, 42, 8, 52, 28, SSD1306_WHITE);
  display.fillTriangle(102, 32, 86, 8, 76, 28, SSD1306_WHITE);
  display.fillTriangle(30, 30, 42, 14, 48, 27, SSD1306_BLACK);
  display.fillTriangle(98, 30, 86, 14, 80, 27, SSD1306_BLACK);

  // Sleeping / happy arched eyes
  display.drawCircle(46, 36, 8, SSD1306_WHITE);
  display.fillRect(36, 36, 20, 10, SSD1306_BLACK);
  display.drawCircle(82, 36, 8, SSD1306_WHITE);
  display.fillRect(72, 36, 20, 10, SSD1306_BLACK);

  // Tip nose
  display.fillTriangle(64, 46, 60, 41, 68, 41, SSD1306_WHITE);

  // Cozy Zzz floating
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(98, 12); display.print("z");
  display.setCursor(106, 6);  display.print("Z");
}

void drawKitty(uint32_t t = 0) {
  // Triangular cat ears
  display.fillTriangle(30, 30, 40, 10, 52, 24, SSD1306_WHITE);
  display.fillTriangle(98, 30, 88, 10, 76, 24, SSD1306_WHITE);
  display.fillTriangle(33, 28, 40, 14, 48, 24, SSD1306_BLACK);
  display.fillTriangle(95, 28, 88, 14, 80, 24, SSD1306_BLACK);

  // Head
  display.drawRoundRect(32, 18, 64, 42, 16, SSD1306_WHITE);

  // Large round anime eyes
  display.fillCircle(46, 33, 6, SSD1306_WHITE);
  display.fillCircle(82, 33, 6, SSD1306_WHITE);
  display.fillCircle(45, 31, 2, SSD1306_BLACK);
  display.fillCircle(81, 31, 2, SSD1306_BLACK);

  // Nose & mouth
  display.drawPixel(64, 42, SSD1306_WHITE);
  display.drawCircle(60, 45, 3, SSD1306_WHITE);
  display.drawCircle(68, 45, 3, SSD1306_WHITE);
  display.fillRect(55, 40, 18, 4, SSD1306_BLACK);

  // Whiskers
  display.drawLine(24, 38, 36, 40, SSD1306_WHITE);
  display.drawLine(24, 44, 36, 43, SSD1306_WHITE);
  display.drawLine(104, 38, 92, 40, SSD1306_WHITE);
  display.drawLine(104, 44, 92, 43, SSD1306_WHITE);
}

void drawPanda(uint32_t t = 0) {
  // Round black panda ears
  display.fillCircle(34, 18, 10, SSD1306_WHITE);
  display.fillCircle(94, 18, 10, SSD1306_WHITE);

  // Head outline
  display.drawRoundRect(30, 14, 68, 46, 18, SSD1306_WHITE);

  // Iconic diagonal panda eye patches
  display.fillRoundRect(40, 26, 14, 12, 5, SSD1306_WHITE);
  display.fillRoundRect(74, 26, 14, 12, 5, SSD1306_WHITE);
  display.fillCircle(47, 32, 2, SSD1306_BLACK);
  display.fillCircle(81, 32, 2, SSD1306_BLACK);

  // Nose & soft smile
  display.fillCircle(64, 40, 3, SSD1306_WHITE);
  display.drawCircle(64, 46, 5, SSD1306_WHITE);
  display.fillRect(56, 40, 16, 5, SSD1306_BLACK);
}

void drawHamster(uint32_t t = 0) {
  // Small round hamster ears
  display.fillCircle(38, 16, 7, SSD1306_WHITE);
  display.fillCircle(90, 16, 7, SSD1306_WHITE);
  display.fillCircle(38, 16, 4, SSD1306_BLACK);
  display.fillCircle(90, 16, 4, SSD1306_BLACK);

  // Chubby cheeks body
  display.drawRoundRect(30, 16, 68, 44, 18, SSD1306_WHITE);

  // Sparkly bead eyes
  display.fillCircle(46, 32, 5, SSD1306_WHITE);
  display.fillCircle(82, 32, 5, SSD1306_WHITE);
  display.fillCircle(45, 30, 2, SSD1306_BLACK);
  display.fillCircle(81, 30, 2, SSD1306_BLACK);

  // Little mouth
  display.fillTriangle(64, 38, 62, 36, 66, 36, SSD1306_WHITE);
  display.drawPixel(64, 42, SSD1306_WHITE);
  display.drawPixel(63, 43, SSD1306_WHITE);
  display.drawPixel(65, 43, SSD1306_WHITE);

  // Puffed blush cheeks
  display.drawCircle(36, 40, 4, SSD1306_WHITE);
  display.drawCircle(92, 40, 4, SSD1306_WHITE);
}

void drawBunny(uint32_t t = 0) {
  // Tall bunny ears
  display.fillRoundRect(42, 4, 10, 26, 5, SSD1306_WHITE);
  display.fillRoundRect(76, 4, 10, 26, 5, SSD1306_WHITE);
  display.fillRoundRect(44, 7, 6, 20, 3, SSD1306_BLACK);
  display.fillRoundRect(78, 7, 6, 20, 3, SSD1306_BLACK);

  // Face
  display.drawRoundRect(34, 20, 60, 40, 16, SSD1306_WHITE);

  // Gentle eyes
  display.fillCircle(48, 36, 5, SSD1306_WHITE);
  display.fillCircle(80, 36, 5, SSD1306_WHITE);
  display.fillCircle(47, 34, 2, SSD1306_BLACK);
  display.fillCircle(79, 34, 2, SSD1306_BLACK);

  // Bunny Y-mouth
  display.fillTriangle(64, 43, 62, 41, 66, 41, SSD1306_WHITE);
  display.drawLine(64, 43, 64, 47, SSD1306_WHITE);
  display.drawLine(64, 47, 60, 50, SSD1306_WHITE);
  display.drawLine(64, 47, 68, 50, SSD1306_WHITE);
}

void drawHappy(uint32_t t = 0) {
  display.drawCircle(42, 28, 10, SSD1306_WHITE);
  display.fillRect(30, 28, 24, 14, SSD1306_BLACK);
  display.drawCircle(86, 28, 10, SSD1306_WHITE);
  display.fillRect(74, 28, 24, 14, SSD1306_BLACK);

  display.drawCircle(64, 34, 16, SSD1306_WHITE);
  display.fillRect(46, 20, 36, 17, SSD1306_BLACK);

  display.fillCircle(28, 38, 4, SSD1306_WHITE);
  display.fillCircle(100, 38, 4, SSD1306_WHITE);
}

void drawWink(uint32_t t = 0) {
  // Left eye winking (star)
  drawStar(42, 28, 9);
  // Right eye open
  display.drawCircle(86, 28, 9, SSD1306_WHITE);
  display.fillCircle(86, 28, 4, SSD1306_WHITE);

  // Playful smirk
  display.drawCircle(66, 38, 10, SSD1306_WHITE);
  display.fillRect(52, 28, 28, 12, SSD1306_BLACK);
}

void drawLoving(uint32_t t = 0) {
  // Big heart eyes!
  drawHeart(42, 26, 14, true);
  drawHeart(86, 26, 14, true);

  // Warm happy smile
  display.drawCircle(64, 36, 14, SSD1306_WHITE);
  display.fillRect(48, 22, 32, 15, SSD1306_BLACK);
}

void drawSleepy(uint32_t t = 0) {
  display.drawCircle(42, 32, 10, SSD1306_WHITE);
  display.fillRect(30, 22, 24, 11, SSD1306_BLACK);
  display.drawCircle(86, 32, 10, SSD1306_WHITE);
  display.fillRect(74, 22, 24, 11, SSD1306_BLACK);

  display.drawPixel(64, 45, SSD1306_WHITE);
  display.drawPixel(63, 45, SSD1306_WHITE);
  display.drawPixel(65, 45, SSD1306_WHITE);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(96, 16); display.print("z");
  display.setCursor(105, 9);  display.print("Z");
  display.setCursor(115, 2);  display.print("Z");
}

void renderFaceByIndex(uint8_t index, uint32_t t = 0) {
  switch (index) {
    case 0: drawPuppy(t); break;
    case 1: drawFox(t); break;
    case 2: drawKitty(t); break;
    case 3: drawPanda(t); break;
    case 4: drawHamster(t); break;
    case 5: drawBunny(t); break;
    case 6: drawHappy(t); break;
    case 7: drawWink(t); break;
    case 8: drawLoving(t); break;
    case 9: drawSleepy(t); break;
    case 10: 
      if (hasCustomBitmap) display.drawBitmap(0, 0, customBitmap, 128, 64, SSD1306_WHITE);
      else drawHappy(t);
      break;
    default: drawHappy(t); break;
  }
}

// ==========================================
// 💓 REACTION ENGINES (SINGLE & DOUBLE TAP)
// ==========================================
void renderReactionFrame() {
  display.clearDisplay();

  if (currentMode == MODE_DOUBLE_TAP) {
    // 🌟 DOUBLE-TAP GESTURE: Massive Radiant Heart Explosion!
    int pulse = (animTick * 3) % 40;
    drawHeart(64, 30, 10 + pulse, false);
    drawHeart(64, 30, 6 + (pulse / 2), true);

    // Shooting stars
    drawStar(20, 20 + (animTick % 20), 4);
    drawStar(108, 20 + ((animTick + 10) % 20), 4);
    drawStar(30, 50 - (animTick % 25), 3);
    drawStar(98, 50 - ((animTick + 7) % 25), 3);

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(32, 54);
    display.print(F("<3 LOVE YOU <3"));
  } else {
    // 🐾 SINGLE-TAP: Face-specific personality reaction!
    renderFaceByIndex(currentFaceIndex, animTick);

    // Floating heart from nose/cheeks
    int heartY = 36 - ((animTick * 2) % 32);
    int heartX = 64 + (int)(sin(animTick * 0.5) * 10);
    drawHeart(heartX, heartY, 8, true);

    if (animTick % 6 < 3) {
      drawStar(20, 16, 4);
      drawStar(108, 16, 4);
    }
  }

  display.display();
}

// Word-wrapped text renderer for messages
void renderWrappedMessage(const String &text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Border frame
  display.drawRoundRect(2, 2, 124, 60, 6, SSD1306_WHITE);
  display.setCursor(10, 8);
  display.print(F("=== REMINDER ==="));

  int y = 22;
  int start = 0;
  while (start < text.length() && y < 52) {
    String line = text.substring(start, min((int)text.length(), start + 18));
    display.setCursor(10, y);
    display.print(line);
    y += 10;
    start += 18;
  }
  display.display();
}

// ==========================================
// 📋 ON-SCREEN OLED MENU SYSTEM
// ==========================================
void renderMenu() {
  display.clearDisplay();

  // Header banner
  display.fillRect(0, 0, 128, 11, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(14, 2);

  if (currentMenuLevel == MENU_MAIN) {
    display.print(F("=== MENU SELECT ==="));
    const char* items[] = {
      "1. Cute Pets (6)",
      "2. Expressions (4)",
      "3. Show Message",
      "4. Settings & Power",
      "5. [ Close Menu ]"
    };
    uint8_t count = 5;

    display.setTextSize(1);
    for (uint8_t i = 0; i < count && i < 4; i++) {
      uint8_t idx = i + menuScroll;
      if (idx >= count) break;
      uint8_t y = 14 + (i * 12);
      bool isSelected = (idx == menuCursor);

      if (isSelected) {
        display.fillRect(0, y - 1, 128, 11, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(2, y);
        display.print("> ");
        display.print(items[idx]);
      } else {
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(10, y);
        display.print(items[idx]);
      }
    }
  } else if (currentMenuLevel == MENU_SETTINGS) {
    display.print(F("=== SETTINGS ==="));
    String items[4];
    items[0] = String("AutoSleep: [") + (autoSleepEnabled ? "ON" : "OFF") + "]";
    items[1] = String("Wi-Fi AP:  [") + (wifiEnabled ? "ON" : "OFF") + "]";
    items[2] = String("DoubleTap: [") + (doubleTapEnabled ? "ON" : "OFF") + "]";
    items[3] = "< Back to Menu";
    uint8_t count = 4;

    display.setTextSize(1);
    for (uint8_t i = 0; i < count; i++) {
      uint8_t y = 14 + (i * 12);
      bool isSelected = (i == menuCursor);

      if (isSelected) {
        display.fillRect(0, y - 1, 128, 11, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(2, y);
        display.print("> ");
        display.print(items[i]);
      } else {
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(10, y);
        display.print(items[i]);
      }
    }
  } else if (currentMenuLevel == MENU_PETS) {
    display.print(F("=== CUTE PETS ==="));
    const char* items[] = { "Puppy", "Cozy Fox", "Kitty", "Panda", "Hamster", "Bunny", "< Back" };
    uint8_t count = 7;
    for (uint8_t i = 0; i < 4; i++) {
      uint8_t idx = i + menuScroll;
      if (idx >= count) break;
      uint8_t y = 14 + (i * 12);
      bool isSelected = (idx == menuCursor);

      if (isSelected) {
        display.fillRect(0, y - 1, 128, 11, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(2, y);
        display.print("> ");
        display.print(items[idx]);
      } else {
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(10, y);
        display.print(items[idx]);
      }
    }
  } else if (currentMenuLevel == MENU_EXPR) {
    display.print(F("=== EXPRESSIONS ==="));
    const char* items[] = { "Happy Smiles", "Wink Star", "Loving Hearts", "Sweet Sleepy", "< Back" };
    uint8_t count = 5;
    for (uint8_t i = 0; i < count; i++) {
      uint8_t y = 14 + (i * 12);
      bool isSelected = (i == menuCursor);

      if (isSelected) {
        display.fillRect(0, y - 1, 128, 11, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(2, y);
        display.print("> ");
        display.print(items[i]);
      } else {
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(10, y);
        display.print(items[i]);
      }
    }
  }

  display.display();
}

// ==========================================
// 🌙 POWER & SLEEP MANAGEMENT
// ==========================================
void goToSleep() {
  if (isSleeping) return;
  isSleeping = true;
  display.clearDisplay();
  display.display();
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  Serial.println(F("[Power] Entering OLED Auto-Sleep... Touch/Button to wake."));
}

void wakeUp() {
  if (!isSleeping) return;
  isSleeping = false;
  display.ssd1306_command(SSD1306_DISPLAYON);
  lastActivityMillis = millis();
  Serial.println(F("[Power] Woke up from sleep!"));

  // Show happy greeting
  display.clearDisplay();
  drawHeart(64, 26, 12, true);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(28, 48);
  display.print(F("I Missed You!"));
  display.display();
  delay(600);

  currentMode = MODE_FACE;
  renderCurrent();
}

void updateWiFiState() {
  if (wifiEnabled) {
    if (strlen(HOME_WIFI_SSID) > 0) {
      Serial.print(F("[WiFi] Connecting to Home Wi-Fi: "));
      Serial.println(HOME_WIFI_SSID);
      WiFi.mode(WIFI_STA);
      WiFi.begin(HOME_WIFI_SSID, HOME_WIFI_PASS);
      int retry = 0;
      while (WiFi.status() != WL_CONNECTED && retry < 15) {
        delay(300);
        Serial.print(F("."));
        retry++;
      }
      if (WiFi.status() == WL_CONNECTED) {
        wifiIP = WiFi.localIP().toString();
        Serial.print(F("\n[WiFi] Connected to Home Wi-Fi! IP: "));
        Serial.println(wifiIP);
      } else {
        Serial.println(F("\n[WiFi] Home Wi-Fi failed. Falling back to Hotspot AP..."));
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, AP_PASS);
        wifiIP = WiFi.softAPIP().toString();
      }
    } else {
      Serial.println(F("[WiFi] Starting Hotspot AP: LittleReminder (192.168.4.1)..."));
      WiFi.mode(WIFI_AP);
      WiFi.softAP(AP_SSID, AP_PASS);
      wifiIP = WiFi.softAPIP().toString();
    }

    // 1. Mobile Web App Dashboard
    server.on("/", []() {
      server.send(200, "text/html", R"rawliteral(<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Little Reminder Companion</title>
  <style>
    * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
    body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; background: #FFFDF9; color: #2D2638; margin: 0; padding: 20px; text-align: center; }
    .card { background: white; border-radius: 24px; padding: 18px; margin-bottom: 16px; border: 1px solid #F0ECE4; box-shadow: 0 4px 15px rgba(0,0,0,0.03); }
    h2 { margin: 6px 0 14px; font-size: 20px; color: #2D2638; }
    h3 { margin: 0 0 10px; font-size: 14px; color: #7B6F88; text-transform: uppercase; letter-spacing: 0.5px; }
    .btn-heart { background: linear-gradient(135deg, #FF6584, #FF8E9E); color: white; border: none; padding: 14px 28px; border-radius: 30px; font-size: 16px; font-weight: bold; cursor: pointer; box-shadow: 0 4px 12px rgba(255,101,132,0.3); width: 100%; transition: transform 0.1s; }
    .btn-heart:active { transform: scale(0.97); }
    .grid { display: grid; grid-cols: 2; grid-template-columns: repeat(2, 1fr); gap: 10px; margin-top: 10px; }
    .btn-face { background: #FAF7F2; border: 1px solid #EBE4DA; border-radius: 16px; padding: 12px 8px; font-size: 14px; font-weight: 600; color: #4A3E56; cursor: pointer; display: flex; align-items: center; justify-content: center; gap: 6px; }
    .btn-face:active { background: #EFE8DC; }
    input, textarea { width: 100%; padding: 12px; border: 1px solid #EBE4DA; border-radius: 14px; font-size: 14px; margin-bottom: 10px; background: #FAF7F2; font-family: inherit; }
    textarea { font-family: monospace; font-size: 11px; height: 90px; resize: vertical; }
    .btn-action { background: #5E5072; color: white; border: none; padding: 12px 20px; border-radius: 16px; font-size: 14px; font-weight: bold; width: 100%; cursor: pointer; }
    .toast { position: fixed; bottom: 20px; left: 50%; transform: translateX(-50%); background: #2D2638; color: white; padding: 10px 20px; border-radius: 20px; font-size: 13px; display: none; opacity: 0; transition: opacity 0.3s; }
  </style>
</head>
<body>
  <h2>🐾 Little Reminder</h2>
  
  <div class="card">
    <button class="btn-heart" onclick="sendPat()">Send Love / Pat ♥</button>
  </div>

  <div class="card">
    <h3>Switch Pet or Face</h3>
    <div class="grid">
      <button class="btn-face" onclick="setFace(0)">🐶 Puppy</button>
      <button class="btn-face" onclick="setFace(1)">🦊 Fox</button>
      <button class="btn-face" onclick="setFace(2)">🐱 Kitty</button>
      <button class="btn-face" onclick="setFace(3)">🐼 Panda</button>
      <button class="btn-face" onclick="setFace(4)">🐹 Hamster</button>
      <button class="btn-face" onclick="setFace(5)">🐰 Bunny</button>
      <button class="btn-face" onclick="setFace(6)">😊 Happy</button>
      <button class="btn-face" onclick="setFace(7)">😉 Wink</button>
      <button class="btn-face" onclick="setFace(8)">💕 Loving</button>
      <button class="btn-face" onclick="setFace(9)">😴 Sleepy</button>
    </div>
  </div>

  <div class="card">
    <h3>Send Reminder to OLED</h3>
    <input type="text" id="msgText" placeholder="e.g. You are loved today!">
    <button class="btn-action" onclick="sendMsg()">Display Message</button>
  </div>

  <div class="card">
    <h3>Add New Face (.INO Code)</h3>
    <textarea id="inoCode" placeholder="Paste const uint8_t frame[1024] = { 0x.. } from oledanimationmaker.com here..."></textarea>
    <button class="btn-action" onclick="uploadIno()">Upload Face to OLED</button>
  </div>

  <div id="toast" class="toast"></div>

  <script>
    function showToast(msg) {
      const t = document.getElementById('toast');
      t.innerText = msg;
      t.style.display = 'block';
      t.style.opacity = '1';
      setTimeout(() => { t.style.opacity = '0'; setTimeout(() => t.style.display = 'none', 300); }, 1800);
    }
    function sendPat() {
      fetch('/pat').then(() => showToast('♥ Sent Love Reaction!'));
    }
    function setFace(id) {
      fetch('/setFace?id=' + id).then(() => showToast('Face Switched!'));
    }
    function sendMsg() {
      const txt = document.getElementById('msgText').value.trim();
      if (!txt) return;
      fetch('/sendMsg?text=' + encodeURIComponent(txt)).then(() => showToast('Message Sent to OLED!'));
    }
    function uploadIno() {
      const code = document.getElementById('inoCode').value;
      if (!code) return;
      showToast('Uploading bitmap...');
      fetch('/uploadIno', { method: 'POST', body: code })
        .then(r => r.text())
        .then(res => showToast(res))
        .catch(() => showToast('Upload failed!'));
    }
  </script>
</body>
</html>)rawliteral");
    });

    // 2. API Endpoints
    server.on("/pat", []() {
      triggerReaction(false);
      server.send(200, "text/plain", "OK");
    });

    server.on("/setFace", []() {
      if (server.hasArg("id")) {
        currentFaceIndex = server.arg("id").toInt() % 10;
        currentMode = MODE_FACE;
        renderCurrent();
        server.send(200, "text/plain", "OK");
      } else {
        server.send(400, "text/plain", "Missing id");
      }
    });

    server.on("/sendMsg", []() {
      if (server.hasArg("text")) {
        currentMessageText = server.arg("text");
        currentMode = MODE_MESSAGE;
        renderCurrent();
        server.send(200, "text/plain", "OK");
      } else {
        server.send(400, "text/plain", "Missing text");
      }
    });

    server.on("/uploadIno", HTTP_POST, []() {
      String body = server.arg("plain");
      int byteIdx = 0;
      int searchPos = 0;
      while (byteIdx < 1024) {
        int hexStart = body.indexOf("0x", searchPos);
        int hexStartUpper = body.indexOf("0X", searchPos);
        int pos = -1;
        if (hexStart != -1 && hexStartUpper != -1) pos = min(hexStart, hexStartUpper);
        else if (hexStart != -1) pos = hexStart;
        else if (hexStartUpper != -1) pos = hexStartUpper;
        if (pos == -1) break;

        String hexByte = body.substring(pos, pos + 4);
        customBitmap[byteIdx++] = (uint8_t)strtol(hexByte.c_str(), NULL, 16);
        searchPos = pos + 2;
      }

      if (byteIdx == 1024) {
        // Auto-polarity check: If >400 0xFFs, invert to crisp white on black
        int ffCount = 0;
        for (int i = 0; i < 1024; i++) if (customBitmap[i] == 0xFF) ffCount++;
        if (ffCount > 400) {
          for (int i = 0; i < 1024; i++) customBitmap[i] = ~customBitmap[i];
        }
        hasCustomBitmap = true;
        currentFaceIndex = 10; // Custom bitmap index
        currentMode = MODE_FACE;
        renderCurrent();
        server.send(200, "text/plain", "Custom Face Loaded on OLED!");
      } else {
        server.send(400, "text/plain", "Need 1024 bytes (found " + String(byteIdx) + ")");
      }
    });

    server.begin();
  } else {
    Serial.println(F("[WiFi] Turning OFF Wi-Fi to maximize battery..."));
    server.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
  }
}

// ==========================================
// 🔘 HARDWARE BUTTON & TOUCH HANDLERS
// ==========================================
void handleButtons() {
  // Read buttons (Active LOW with internal pullups)
  bool btn1 = (digitalRead(BTN1_PIN) == LOW);
  bool btn2 = (digitalRead(BTN2_PIN) == LOW);

  if (btn1 || btn2) {
    if (isSleeping) {
      wakeUp();
      delay(250);
      return;
    }
    lastActivityMillis = millis();
  }

  // Button 1: Face Cycle / Menu Down
  static bool btn1Last = false;
  if (btn1 && !btn1Last) {
    delay(40); // Debounce
    if (currentMode == MODE_MENU) {
      // Menu cursor down
      uint8_t maxItems = 5;
      if (currentMenuLevel == MENU_SETTINGS) maxItems = 4;
      else if (currentMenuLevel == MENU_PETS) maxItems = 7;
      else if (currentMenuLevel == MENU_EXPR) maxItems = 5;

      menuCursor = (menuCursor + 1) % maxItems;
      if (menuCursor >= 4) menuScroll = menuCursor - 3;
      else menuScroll = 0;
      renderMenu();
    } else {
      // Cycle next face
      currentFaceIndex = (currentFaceIndex + 1) % TOTAL_FACES;
      currentMode = MODE_FACE;
      renderCurrent();
    }
  }
  btn1Last = btn1;

  // Button 2: Menu Open / Select / Enter
  static bool btn2Last = false;
  if (btn2 && !btn2Last) {
    delay(40); // Debounce
    if (currentMode != MODE_MENU) {
      currentMode = MODE_MENU;
      currentMenuLevel = MENU_MAIN;
      menuCursor = 0;
      menuScroll = 0;
      renderMenu();
    } else {
      // In menu: Select item
      if (currentMenuLevel == MENU_MAIN) {
        if (menuCursor == 0) { currentMenuLevel = MENU_PETS; menuCursor = 0; }
        else if (menuCursor == 1) { currentMenuLevel = MENU_EXPR; menuCursor = 0; }
        else if (menuCursor == 2) { currentMode = MODE_MESSAGE; renderCurrent(); }
        else if (menuCursor == 3) { currentMenuLevel = MENU_SETTINGS; menuCursor = 0; }
        else if (menuCursor == 4) { currentMode = MODE_FACE; renderCurrent(); }
      } else if (currentMenuLevel == MENU_SETTINGS) {
        if (menuCursor == 0) {
          autoSleepEnabled = !autoSleepEnabled; // TOGGLE AUTO SLEEP!
        } else if (menuCursor == 1) {
          wifiEnabled = !wifiEnabled;           // TOGGLE WI-FI!
          updateWiFiState();
        } else if (menuCursor == 2) {
          doubleTapEnabled = !doubleTapEnabled; // TOGGLE DOUBLE TAP!
        } else if (menuCursor == 3) {
          currentMenuLevel = MENU_MAIN;
          menuCursor = 3;
        }
      } else if (currentMenuLevel == MENU_PETS) {
        if (menuCursor < 6) {
          currentFaceIndex = menuCursor;
          currentMode = MODE_FACE;
        } else {
          currentMenuLevel = MENU_MAIN;
          menuCursor = 0;
        }
      } else if (currentMenuLevel == MENU_EXPR) {
        if (menuCursor < 4) {
          currentFaceIndex = 6 + menuCursor;
          currentMode = MODE_FACE;
        } else {
          currentMenuLevel = MENU_MAIN;
          menuCursor = 1;
        }
      }

      if (currentMode == MODE_MENU) renderMenu();
      else renderCurrent();
    }
  }
  btn2Last = btn2;
}

void triggerReaction(bool isDoubleTap) {
  currentMode = isDoubleTap ? MODE_DOUBLE_TAP : MODE_REACTION;
  reactionStartMillis = millis();
  reactionDurationMs = isDoubleTap ? 3200 : 2500;
  animTick = 0;
  renderReactionFrame();
}

void handleTouchGestures() {
  bool isTouched = (digitalRead(TOUCH_PIN) == HIGH);
  uint32_t now = millis();

  if (isTouched) {
    if (isSleeping) {
      wakeUp();
      delay(300);
      return;
    }
    lastActivityMillis = now;

    if (!touchWasPressed) {
      touchWasPressed = true;
      touchDownTime = now;
    } else {
      // Long press check (> 1400ms)
      if (now - touchDownTime > 1400 && currentMode != MODE_MENU) {
        // Long press -> Cozy purr snooze
        currentFaceIndex = 9; // Sleepy
        triggerReaction(false);
        touchWasPressed = false;
        tapCount = 0;
        return;
      }
    }
  } else {
    // Touch released
    if (touchWasPressed) {
      touchWasPressed = false;
      uint32_t pressDuration = now - touchDownTime;

      if (pressDuration < 500) {
        tapCount++;
        lastTapReleaseTime = now;
      }
    }
  }

  // Double tap vs Single tap resolution
  if (tapCount > 0 && (now - lastTapReleaseTime > 320)) {
    if (tapCount >= 2 && doubleTapEnabled) {
      Serial.println(F("[Touch] DOUBLE-TAP Detected! Triggering Heart Explosion!"));
      triggerReaction(true);
    } else {
      Serial.println(F("[Touch] Single Tap Detected. Triggering Face Reaction!"));
      triggerReaction(false);
    }
    tapCount = 0;
  }
}

void renderCurrent() {
  display.clearDisplay();
  if (currentMode == MODE_FACE) {
    renderFaceByIndex(currentFaceIndex, 0);
  } else if (currentMode == MODE_MESSAGE) {
    renderWrappedMessage(currentMessageText);
    return;
  }
  display.display();
}

// ==========================================
// 🚀 ARDUINO SETUP & LOOP
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println(F("\n=== Little Reminder Companion (Option B) ==="));

  // Configure hardware pins
  pinMode(BTN1_PIN, INPUT_PULLUP);
  pinMode(BTN2_PIN, INPUT_PULLUP);
  pinMode(TOUCH_PIN, INPUT_PULLDOWN);

  // Initialize I2C OLED with custom SDA & SCL
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("[Error] SSD1306 allocation failed. Check wiring!"));
    while (1) delay(100);
  }

  display.clearDisplay();

  // Splash Screen
  drawHeart(64, 18, 14, true);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(22, 38);
  display.print(F("Little Reminder"));
  display.setCursor(24, 50);
  display.print(F("Battery Pet v2"));
  display.display();
  delay(1000);

  // Start Wi-Fi and show connection details on OLED
  if (wifiEnabled) {
    updateWiFiState();

    display.clearDisplay();
    drawHeart(64, 12, 10, true);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(24, 28);
    display.print(F("Wi-Fi Ready!"));
    display.setCursor(6, 42);
    display.print(strlen(HOME_WIFI_SSID) > 0 ? "Net: " + String(HOME_WIFI_SSID) : "Wi-Fi: LittleReminder");
    display.setCursor(6, 54);
    display.print("http://" + wifiIP);
    display.display();
    delay(2200);
  }

  lastActivityMillis = millis();
  renderCurrent();
}

void loop() {
  uint32_t now = millis();

  // 1. Handle hardware input
  handleButtons();
  handleTouchGestures();

  // 2. Wi-Fi client handling (if enabled)
  if (wifiEnabled) {
    server.handleClient();
  }

  // 3. Reaction animation loop
  if (currentMode == MODE_REACTION || currentMode == MODE_DOUBLE_TAP) {
    if (now - reactionStartMillis > reactionDurationMs) {
      currentMode = MODE_FACE;
      renderCurrent();
    } else {
      if (now - lastAnimTickMillis >= 60) {
        lastAnimTickMillis = now;
        animTick++;
        renderReactionFrame();
      }
    }
    return;
  }

  // 4. Auto-Sleep Battery Saver
  if (autoSleepEnabled && !isSleeping && currentMode != MODE_MENU) {
    if (now - lastActivityMillis > sleepTimeoutMs) {
      goToSleep();
    }
  }

  delay(15);
}