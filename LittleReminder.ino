/*
 * =========================================================================
 * 🐾 CottonKitty / Little Reminder - ESP32-C3 SuperMini + SSD1306 OLED 🐾
 * =========================================================================
 *
 * HARDWARE PIN CONFIGURATION (Locked):
 * - OLED I2C SDA:  GPIO 5
 * - OLED I2C SCL:  GPIO 6 (0x3C, 128x64)
 * - TOUCH SENSOR:  GPIO 3 (Active HIGH, digital touch module OUT)
 * - BUTTON 1:      GPIO 9 (Next Mode / Menu Down - Active LOW with INPUT_PULLUP)
 * - BUTTON 2:      GPIO 1 (Open Menu / Select    - Active LOW with INPUT_PULLUP)
 *
 * FEATURES:
 * 1. 8 Original Gallery Drawings & Multi-Frame Animations from CottonKitty:
 *    - Walking Cat (28 frames)
 *    - Heart-Eyes Face (28 frames)
 *    - Dove with Flower (1 frame)
 *    - Sad Face (10 frames)
 *    - Cat & Mouse Cuddle (1 frame)
 *    - Girl with Umbrella (1 frame)
 *    - Couple High-Five (1 frame)
 *    - Girl Portrait (1 frame)
 * 2. 32 Procedural Expressive Kaomoji Faces with blinking, dancing & reactions
 * 3. On-Screen OLED Menu System (Navigated via BTN1 and BTN2):
 *    - Gallery Drawings Sub-Menu
 *    - Cute Expressions Sub-Menu
 *    - Preset Messages Sub-Menu
 *    - Settings & Power (Auto-Sleep, Wi-Fi toggle, Double-Tap toggle)
 * 4. Interactive Touch Sensor Gestures:
 *    - Single Tap: Love Heart Reaction
 *    - Double Tap: Animated Heart Explosion Burst!
 *    - Long Press (>1.5s): Snooze / Sleepy Zzz mode
 * 5. Auto-Sleep Battery Saver: Turns OLED display off after 45s of inactivity.
 *    Instant wake on touch or button press!
 * 6. Dual Control:
 *    - USB-C OTG Serial (Web Serial direct cable connection)
 *    - Wi-Fi Web Server (STA connect to 'ESP' with AP fallback 'CottonKitty')
 * =========================================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>
#include "GalleryBitmaps.h"

// ==========================================
// 📌 PIN DEFINITIONS
// ==========================================
#define OLED_SDA       5
#define OLED_SCL       6
#define TOUCH_PIN      3
#define BTN1_PIN       9   // Next Mode / Menu Down
#define BTN2_PIN       1   // Menu Open / Select

#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define OLED_ADDR      0x3C

// ==========================================
// 📶 WI-FI CREDENTIALS
// ==========================================
const char* HOME_WIFI_SSID = "ESP";
const char* HOME_WIFI_PASS = "Ajay@esp";
const char* AP_SSID        = "CottonKitty";
const char* AP_PASS        = "kittyfriend";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WebServer server(80);

// ==========================================
// 🐾 MODES & EXPRESSIONS
// ==========================================
const char* ALL_MODES[] = {
  "happy", "love", "shy", "sleep", "laugh", "cry", "angry", "surprise",
  "wink", "cool", "sad", "blush", "excited", "dance", "thinking", "confused",
  "ghost", "bunny", "star", "heart", "sleepy", "proud", "oops", "relaxed",
  "sparkle", "tiny", "robot", "kiss", "pout", "party", "floating", "calm",
  "gallery_catwalk", "gallery_heartface", "gallery_dove", "gallery_sadface",
  "gallery_catmouse", "gallery_rainygirl", "gallery_couple", "gallery_girlportrait"
};
const uint8_t TOTAL_MODES = 40;

String currentMode = "happy";
String currentMessage = "You matter.";
uint8_t modeIdx = 0;
uint8_t frameIdx = 0;
uint8_t greetingIdx = 0;
unsigned long lastFrame = 0;
String wifiIP = "192.168.4.1";

const char* GREETINGS[] = {
  "You matter.",
  "You make me smile.",
  "A little hug for you.",
  "You are appreciated.",
  "Keep shining."
};
const uint8_t TOTAL_GREETINGS = 5;

// ==========================================
// 📋 MENU DEFINITIONS
// ==========================================
enum MenuState {
  STATE_NORMAL = 0,
  MENU_MAIN,
  MENU_GALLERY,
  MENU_EXPRESSIONS,
  MENU_MESSAGES,
  MENU_SETTINGS
};

MenuState currentMenuState = STATE_NORMAL;
int8_t menuCursor = 0;
int8_t menuScroll = 0;

// ==========================================
// ⚙️ SETTINGS & POWER MANAGEMENT
// ==========================================
bool autoSleepEnabled = true;
bool wifiEnabled      = true;
bool doubleTapEnabled = true;

bool isSleeping       = false;
unsigned long lastActivityTime = 0;
const unsigned long AUTO_SLEEP_TIMEOUT = 45000; // 45 seconds

// ==========================================
// 💖 TOUCH & REACTION STATE
// ==========================================
enum ReactionType {
  REACTION_NONE = 0,
  REACTION_LOVE,
  REACTION_BURST,
  REACTION_SNOOZE
};

ReactionType currentReaction = REACTION_NONE;
unsigned long reactionStartTime = 0;
const unsigned long REACTION_DURATION = 2000;

// Double tap detection
unsigned long lastTouchReleaseTime = 0;
unsigned long touchPressStartTime  = 0;
bool touchWasPressed = false;

// Button debounce tracking
bool btn1LastState = HIGH;
bool btn2LastState = HIGH;
unsigned long btn1LastDebounceTime = 0;
unsigned long btn2LastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 40;

// Forward Declarations
void renderMenu();
void drawMode();
void handleTouch();
void handleButtons();
void handleSerial();
void wakeUp();
void goToSleep();
void setupWiFi();

// ==========================================
// 🌐 WEB APPLICATION HTML (Stored in Flash)
// ==========================================
const char APP_HTML[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>CottonKitty & Little Reminder</title>
<style>
:root{font-family:system-ui,-apple-system,BlinkMacSystemFont,Segoe UI,Roboto,sans-serif;color:#252632;background:#f8f8fc}
*{box-sizing:border-box}
body{margin:0}
main{max-width:960px;margin:auto;padding:20px 16px}
header{display:flex;align-items:center;gap:14px;margin-bottom:20px}
.logo{background:#ffe8f1;border-radius:18px;padding:12px 16px;font-size:28px}
h1{font-size:24px;margin:0}
p{color:#727582;margin:4px 0}
.layout{display:grid;grid-template-columns:1fr 1.3fr;gap:18px}
.card{background:white;border:1px solid #e8e8f0;border-radius:20px;padding:18px;margin-bottom:16px;box-shadow:0 2px 8px rgba(0,0,0,0.03)}
h2{font-size:17px;margin:0 0 12px;display:flex;align-items:center;gap:8px}
button,input,textarea{font:inherit;border-radius:12px;padding:11px;border:1px solid #e5e5ed}
button{cursor:pointer;background:#292a35;color:white;border:0;font-weight:500;transition:transform 0.1s,background 0.2s}
button:active{transform:scale(0.97)}
button.alt{background:#f1eff7;color:#292a35}
button.pink{background:#f6c2d7;color:#34232b}
button.usb{background:#4f46e5;color:white}
button.usb.connected{background:#059669}
input,textarea{width:100%}
.row{display:flex;gap:8px;margin:8px 0}
.row>*{flex:1}
.status{font-size:13px;color:#747582;margin-top:10px;padding:8px 12px;background:#f9f9fc;border-radius:10px}
.preview{background:#08090d;color:#00ffcc;border-radius:16px;padding:24px 15px;text-align:center;margin-bottom:12px;min-height:120px;display:flex;flex-direction:column;justify-content:center;align-items:center;box-shadow:inset 0 0 15px rgba(0,255,204,0.15)}
.big{font-size:36px;font-family:monospace;letter-spacing:1px}
.label{font-size:14px;color:#8be9fd;margin-top:8px}
.gallery{display:grid;grid-template-columns:repeat(2,1fr);gap:8px;margin-top:10px}
.gallery button{background:#fff;color:#292a35;border:1px solid #e8e8f0;text-align:left;padding:12px;min-height:56px;display:flex;align-items:center;gap:10px}
.gallery button.active{background:#fff0f6;border-color:#e99cba}
.faces{display:grid;grid-template-columns:repeat(4,1fr);gap:8px}
.face{background:#fff;color:#292a35;border:1px solid #e8e8f0;display:flex;flex-direction:column;align-items:center;gap:4px;padding:10px 4px;min-height:76px}
.face span{font-size:22px}
.face small{font-size:11px;color:#727582}
.face.active{background:#fff0f6;border:1px solid #e99cba}
.small{font-size:12px;color:#8a8c99}
@media(max-width:680px){
  .layout{grid-template-columns:1fr}
  .faces{grid-template-columns:repeat(3,1fr)}
  .gallery{grid-template-columns:1fr}
}
</style>
</head>
<body>
<main>
<header>
  <div class="logo">♡</div>
  <div>
    <h1>CottonKitty & Little Reminder</h1>
    <p>Your tiny animated companion with OLED Menu & OTG Web Serial</p>
  </div>
</header>

<div class="layout">
  <section>
    <!-- Connection Options -->
    <div class="card">
      <h2>🔌 Connect to Companion</h2>
      <p>Choose <b>USB-C OTG Cable</b> (fastest, zero Wi-Fi needed) or <b>Wi-Fi</b>:</p>
      
      <div style="margin:10px 0">
        <button id="usbBtn" class="usb" style="width:100%" onclick="toggleUSB()">⚡ Connect USB-C OTG Cable</button>
      </div>

      <div class="row">
        <input id="host" value="http://192.168.4.1">
        <button onclick="connectWiFi()">Wi-Fi Connect</button>
      </div>
      <div id="status" class="status">Ready to connect via USB-C or Wi-Fi</div>
    </div>

    <!-- Send a Message -->
    <div class="card">
      <h2>💌 Send a Message to OLED</h2>
      <textarea id="msg" maxlength="120" rows="2" placeholder="You make ordinary days feel special..."></textarea>
      <div class="row">
        <button class="pink" onclick="sendMsg()">Send to OLED</button>
        <button class="alt" onclick="presetMsg('You matter. ♡')">You matter ♡</button>
      </div>
      <div class="row">
        <button class="alt" onclick="presetMsg('Keep shining! ✨')">Keep shining ✨</button>
        <button class="alt" onclick="presetMsg('A little hug for you! 🐾')">Hug 🐾</button>
      </div>
    </div>

    <!-- 8 Original Drawings/Animations Gallery -->
    <div class="card">
      <h2>🖼️ Original Drawings Gallery</h2>
      <p>Multi-frame animations & full-screen drawings from CottonKitty.</p>
      <div class="gallery" id="gallery"></div>
    </div>

    <!-- Quick Reactions -->
    <div class="card">
      <h2>✨ Quick Reactions & Gestures</h2>
      <div class="row">
        <button class="pink" onclick="setMode('love')">♡ Love Hearts</button>
        <button class="alt" onclick="sendPat()">🐾 Pat / Touch</button>
      </div>
      <div class="row">
        <button class="alt" onclick="sendBurst()">💥 Double-Tap Burst</button>
        <button class="alt" onclick="setMode('dance')">♪ Dance</button>
      </div>
      <div class="row">
        <button class="alt" onclick="setMode('sleep')">💤 Sleep</button>
        <button class="alt" onclick="setMode('sparkle')">✧ Sparkles</button>
      </div>
    </div>
  </section>

  <!-- Expressions Preview & Grid -->
  <section>
    <div class="card">
      <h2>🐱 Live Preview & Expressions</h2>
      <div class="preview">
        <div class="big" id="preview">૮ ˶ᵔ ᵕ ᵔ˶ ა</div>
        <div class="label" id="label">Happy Kitty</div>
      </div>
      <p class="small">Tap any expression below to display it instantly on your Little Reminder OLED:</p>
      <div class="faces" id="faces"></div>
    </div>
  </section>
</div>

<p class="small" style="text-align:center;margin-top:20px">
  Little Reminder & CottonKitty Unified Firmware · 128x64 SSD1306 · ESP32-C3 SuperMini · 32 Expressions + 8 Gallery Bitmaps
</p>
</main>

<script>
let port = null, writer = null, isUSB = false;

const items = [
  ['happy','Happy','૮ ˶ᵔ ᵕ ᵔ˶ ა'],['love','Love','♥‿♥'],['shy','Shy','⁄ ⁄>⁄ω⁄<⁄ ⁄'],['sleep','Sleepy','= ᴗ = z'],
  ['laugh','Laugh','≧▽≦'],['cry','Crying','ಥ﹏ಥ'],['angry','Angry','ಠ益ಠ'],['surprise','Surprised','⊙０⊙'],
  ['wink','Wink','😉'],['cool','Cool','⌐■_■'],['sad','Sad','｡•́︿•̀｡'],['blush','Blushing','(⁄ ⁄•⁄ω⁄•⁄ ⁄)'],
  ['excited','Excited','✧▽✧'],['dance','Dancing','♪ᕕ(ᐛ)ᕗ'],['thinking','Thinking','(¬_¬ )'],['confused','Confused','(・・?)'],
  ['ghost','Love Ghost','◉♡◉'],['bunny','Bunny','૮₍ ˶ᵔ ᵕ ᵔ˶ ₎ა'],['star','Starry','✦‿✦'],['heart','Heart','♥'],
  ['sleepy','Drowsy','-ᴗ- zZ'],['proud','Proud','ᕙ( •̀ ᗜ •́ )ᕗ'],['oops','Oops','Σ(°ロ°)'],['relaxed','Relaxed','( ˘ω˘ )'],
  ['sparkle','Sparkles','✧･ﾟ: *✧'],['tiny','Tiny Kitty','ฅ^•ﻌ•^ฅ'],['robot','Robot','[◉_◉]'],['kiss','Kiss','( ˘ ³˘)♥'],
  ['pout','Pout','(｡•́︿•̀｡)'],['party','Party','☆ヾ(≧▽≦*)o'],['floating','Floating','~(˘▾˘~)'],['calm','Calm','(ᵕᴗᵕ)']
];

const galleryItems = [
  ['gallery_catwalk','🐈','Walking cat · 28 frames'],
  ['gallery_heartface','♡','Heart-eyes face · 28 frames'],
  ['gallery_dove','🕊️','Dove with flower'],
  ['gallery_sadface','☹','Sad face · 10 frames'],
  ['gallery_catmouse','🐱','Cat & mouse cuddle'],
  ['gallery_rainygirl','☂','Girl with umbrella'],
  ['gallery_couple','♡','Couple high-five'],
  ['gallery_girlportrait','✿','Girl portrait']
];

// Populate Gallery
const gal = document.getElementById('gallery');
galleryItems.forEach(x => {
  let b = document.createElement('button');
  b.innerHTML = `<span>${x[1]}</span> <div><b>${x[2].split('·')[0]}</b><br><small style="color:#747582">${x[2].split('·')[1]||'Drawing'}</small></div>`;
  b.onclick = () => {
    document.querySelectorAll('#gallery button, .face').forEach(y => y.classList.remove('active'));
    b.classList.add('active');
    document.getElementById('preview').textContent = x[1];
    document.getElementById('label').textContent = x[2];
    setMode(x[0]);
  };
  gal.appendChild(b);
});

// Populate Expressions
const g = document.getElementById('faces');
items.forEach(x => {
  let b = document.createElement('button');
  b.className = 'face';
  b.innerHTML = `<span>${x[2]}</span><small>${x[1]}</small>`;
  b.onclick = () => {
    document.querySelectorAll('#gallery button, .face').forEach(y => y.classList.remove('active'));
    b.classList.add('active');
    document.getElementById('preview').textContent = x[2];
    document.getElementById('label').textContent = x[1];
    setMode(x[0]);
  };
  g.appendChild(b);
});

function base() {
  return document.getElementById('host').value.trim().replace(/\/$/,'');
}

function setStatus(msg) {
  document.getElementById('status').textContent = msg;
}

function presetMsg(t) {
  document.getElementById('msg').value = t;
  sendMsg();
}

// Web Serial OTG Connection
async function toggleUSB() {
  if (isUSB && port) {
    try {
      if (writer) await writer.close();
      await port.close();
    } catch(e) {}
    port = null; writer = null; isUSB = false;
    document.getElementById('usbBtn').className = 'usb';
    document.getElementById('usbBtn').textContent = '⚡ Connect USB-C OTG Cable';
    setStatus('USB-C cable disconnected.');
    return;
  }

  if (!navigator.serial) {
    alert('Web Serial is supported on Chrome/Edge on Mac, PC, Chromebook, and Android with OTG cable!');
    return;
  }

  try {
    port = await navigator.serial.requestPort();
    await port.open({ baudRate: 115200 });
    const textEncoder = new TextEncoderStream();
    textEncoder.readable.pipeTo(port.writable);
    writer = textEncoder.writable.getWriter();
    isUSB = true;
    document.getElementById('usbBtn').className = 'usb connected';
    document.getElementById('usbBtn').textContent = '🟢 USB-C OTG Connected!';
    setStatus('🟢 Connected directly via USB-C OTG cable! Zero Wi-Fi needed.');
    sendSerial('status\n');
  } catch(e) {
    setStatus('USB-C connection cancelled or failed.');
  }
}

async function sendSerial(str) {
  if (writer) {
    try {
      await writer.write(str);
      return true;
    } catch(e) {
      console.error(e);
    }
  }
  return false;
}

// Wi-Fi HTTP Connection
async function connectWiFi() {
  setStatus('Connecting to ' + base() + '...');
  try {
    let r = await fetch(base() + '/api/status');
    if (!r.ok) throw 0;
    let data = await r.json();
    setStatus('🟢 Connected to ' + data.name + ' over Wi-Fi! Mode: ' + data.mode);
  } catch(e) {
    setStatus('Connection failed. Connect to Wi-Fi CottonKitty (kittyfriend) or ESP.');
  }
}

// Unified Dispatchers
async function setMode(n) {
  if (isUSB) {
    await sendSerial('mode:' + n + '\n');
    setStatus('⚡ Sent mode via USB-C OTG: ' + n);
    return;
  }
  try {
    await fetch(base() + '/api/mode?name=' + encodeURIComponent(n));
    setStatus('Sent mode to OLED: ' + n);
  } catch(e) {
    setStatus('Not connected. Use USB-C OTG Cable or Wi-Fi.');
  }
}

async function sendMsg() {
  let t = document.getElementById('msg').value.trim();
  if (!t) return;
  if (isUSB) {
    await sendSerial('msg:' + t + '\n');
    setStatus('⚡ Sent message via USB-C OTG: ' + t);
    return;
  }
  try {
    await fetch(base() + '/api/message?text=' + encodeURIComponent(t));
    setStatus('Message sent to OLED!');
  } catch(e) {
    setStatus('Not connected. Use USB-C OTG Cable or Wi-Fi.');
  }
}

async function sendPat() {
  if (isUSB) {
    await sendSerial('pat\n');
    setStatus('⚡ Sent Pat reaction via USB-C OTG');
    return;
  }
  setMode('love');
}

async function sendBurst() {
  if (isUSB) {
    await sendSerial('doubletap\n');
    setStatus('⚡ Sent Heart Explosion via USB-C OTG');
    return;
  }
  setMode('love');
}
</script>
</body>
</html>
)rawliteral";

// ==========================================
// 🎨 OLED DRAWING UTILITIES
// ==========================================
void centered(String s, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.getTextBounds(s, 0, y, &x1, &y1, &w, &h);
  display.setCursor(max(0, (128 - (int)w) / 2), y);
  display.print(s);
}

void star(int x, int y) {
  display.drawLine(x - 3, y, x + 3, y, SSD1306_WHITE);
  display.drawLine(x, y - 3, x, y + 3, SSD1306_WHITE);
}

void heart(int x, int y, int r) {
  display.fillCircle(x - r, y, r, SSD1306_WHITE);
  display.fillCircle(x + r, y, r, SSD1306_WHITE);
  display.fillTriangle(x - 2 * r, y, x + 2 * r, y, x, y + 3 * r, SSD1306_WHITE);
}

// ==========================================
// 🎭 MAIN DISPLAY RENDERING (Modes, Gallery & Expressions)
// ==========================================
void drawMode() {
  unsigned long t = millis();
  display.clearDisplay();

  // 1. Check if an animated reaction overlay is active
  if (currentReaction == REACTION_BURST) {
    unsigned long elapsed = t - reactionStartTime;
    if (elapsed < REACTION_DURATION) {
      uint8_t frame = (elapsed / 80) % 8;
      display.fillRect(10, 10, 108, 44, SSD1306_BLACK);
      display.drawRoundRect(8, 8, 112, 48, 8, SSD1306_WHITE);
      heart(64, 30 - frame, 4 + (frame % 3));
      heart(36 - (frame * 2), 24 + frame, 3);
      heart(92 + (frame * 2), 24 + frame, 3);
      heart(48, 46, 2);
      heart(80, 46, 2);
      star(24, 18);
      star(104, 18);
      centered(F("♡ LOVE BURST! ♡"), 48);
      display.display();
      return;
    } else {
      currentReaction = REACTION_NONE;
    }
  }

  // 2. Gallery Bitmaps (CottonKitty 8 original drawings/animations)
  if (currentMode.startsWith("gallery_") || currentMode == "uploaded") {
    const uint8_t* const* selectedFrames = heartfaceFrames;
    uint8_t selectedCount = heartfaceCount;

    if (currentMode == "gallery_catwalk") {
      selectedFrames = catwalkFrames;
      selectedCount = catwalkCount;
    } else if (currentMode == "gallery_heartface" || currentMode == "uploaded") {
      selectedFrames = heartfaceFrames;
      selectedCount = heartfaceCount;
    } else if (currentMode == "gallery_dove") {
      selectedFrames = doveFrames;
      selectedCount = doveCount;
    } else if (currentMode == "gallery_sadface") {
      selectedFrames = sadfaceFrames;
      selectedCount = sadfaceCount;
    } else if (currentMode == "gallery_catmouse") {
      selectedFrames = catmouseFrames;
      selectedCount = catmouseCount;
    } else if (currentMode == "gallery_rainygirl") {
      selectedFrames = rainygirlFrames;
      selectedCount = rainygirlCount;
    } else if (currentMode == "gallery_couple") {
      selectedFrames = coupleFrames;
      selectedCount = coupleCount;
    } else if (currentMode == "gallery_girlportrait") {
      selectedFrames = girlportraitFrames;
      selectedCount = girlportraitCount;
    }

    if (selectedCount > 0) {
      if (frameIdx >= selectedCount) frameIdx = 0;
      display.drawBitmap(0, 0, selectedFrames[frameIdx], 128, 64, SSD1306_WHITE);
      if (t - lastFrame >= 120) {
        lastFrame = t;
        frameIdx = (frameIdx + 1) % selectedCount;
      }
    }
  }
  // 3. Custom Messages
  else if (currentMode == "message") {
    heart(64, 16, 5);
    String s = currentMessage;
    if (s.length() > 21) s = s.substring(0, 21);
    centered(s, 44);
  }
  // 4. Procedural Expressive Kaomoji Faces (CottonKitty 32 Faces)
  else {
    bool blink = (t % 3000) < 120;

    if (currentMode == "love" || currentMode == "heart" || currentMode == "ghost" || currentMode == "kiss") {
      display.drawRoundRect(29, 16, 70, 39, 13, SSD1306_WHITE);
      display.fillCircle(48, 31, 5, SSD1306_WHITE);
      display.fillCircle(78, 31, 5, SSD1306_WHITE);
      display.fillCircle(48, 31, 2, SSD1306_BLACK);
      display.fillCircle(78, 31, 2, SSD1306_BLACK);
      display.drawLine(55, 42, 63, 47, SSD1306_WHITE);
      display.drawLine(63, 47, 71, 42, SSD1306_WHITE);
      heart(101, 12, 2);
      star(17, 21);
      star(111, 49);
      if (currentReaction == REACTION_LOVE) {
        heart(22, 12, 3);
        heart(106, 26, 3);
      }
    }
    else if (currentMode == "sleep" || currentMode == "sleepy" || currentReaction == REACTION_SNOOZE) {
      display.drawRoundRect(30, 18, 68, 34, 12, SSD1306_WHITE);
      display.drawLine(42, 33, 54, 33, SSD1306_WHITE);
      display.drawLine(74, 33, 86, 33, SSD1306_WHITE);
      centered("z", 20 + (t / 400) % 10);
      centered("Z", 8 + (t / 500) % 8);
      display.drawLine(54, 43, 74, 43, SSD1306_WHITE);
    }
    else if (currentMode == "stars" || currentMode == "sparkle" || currentMode == "star" || currentMode == "bunny" || currentMode == "floating") {
      star(18, 14);
      star(108, 17);
      star(25, 51);
      star(102, 48);
      display.drawRoundRect(42, 20, 44, 33, 12, SSD1306_WHITE);
      display.fillCircle(54, 32, 2, SSD1306_WHITE);
      display.fillCircle(74, 32, 2, SSD1306_WHITE);
      display.drawLine(54, 40, 64, 44, SSD1306_WHITE);
      display.drawLine(64, 44, 74, 40, SSD1306_WHITE);
      heart(64, 8, 2);
    }
    else {
      int bob = (currentMode == "dance" || currentMode == "party" || currentMode == "excited") ? ((t / 180) % 2 * 3) : 0;
      display.drawRoundRect(30, 16 + bob, 68, 39, 12, SSD1306_WHITE);

      if (currentMode == "angry" || currentMode == "pout") {
        display.drawLine(41, 25 + bob, 55, 31 + bob, SSD1306_WHITE);
        display.drawLine(73, 31 + bob, 87, 25 + bob, SSD1306_WHITE);
      }
      else if (currentMode == "cry" || currentMode == "sad") {
        display.drawLine(42, 30 + bob, 55, 34 + bob, SSD1306_WHITE);
        display.drawLine(73, 34 + bob, 86, 30 + bob, SSD1306_WHITE);
        display.drawLine(48, 35, 45, 44, SSD1306_WHITE);
        display.drawLine(80, 35, 83, 44, SSD1306_WHITE);
      }
      else if (blink) {
        display.drawLine(43, 32 + bob, 55, 32 + bob, SSD1306_WHITE);
        display.drawLine(73, 32 + bob, 85, 32 + bob, SSD1306_WHITE);
      }
      else {
        display.fillRoundRect(43, 25 + bob, 12, 15, 4, SSD1306_WHITE);
        display.fillRoundRect(73, 25 + bob, 12, 15, 4, SSD1306_WHITE);
      }

      if (currentMode == "surprise" || currentMode == "oops") {
        display.drawCircle(64, 44 + bob, 5, SSD1306_WHITE);
      } else {
        display.drawLine(54, 43 + bob, 60, 47 + bob, SSD1306_WHITE);
        display.drawLine(60, 47 + bob, 68, 47 + bob, SSD1306_WHITE);
        display.drawLine(68, 47 + bob, 74, 43 + bob, SSD1306_WHITE);
      }

      if (currentMode == "shy" || currentMode == "blush") {
        display.drawLine(36, 39, 41, 39, SSD1306_WHITE);
        display.drawLine(87, 39, 92, 39, SSD1306_WHITE);
      }
      if (currentMode == "thinking" || currentMode == "confused") {
        centered("?", 3);
      }
      if (currentMode == "cool") {
        display.drawRect(40, 25, 18, 12, SSD1306_WHITE);
        display.drawRect(70, 25, 18, 12, SSD1306_WHITE);
        display.drawLine(58, 30, 70, 30, SSD1306_WHITE);
      }
      if (currentMode == "robot") {
        display.drawRect(40, 24, 18, 15, SSD1306_WHITE);
        display.drawRect(70, 24, 18, 15, SSD1306_WHITE);
      }
      if (currentMode == "tiny") {
        display.drawLine(36, 19, 45, 10, SSD1306_WHITE);
        display.drawLine(45, 10, 53, 19, SSD1306_WHITE);
        display.drawLine(75, 19, 83, 10, SSD1306_WHITE);
        display.drawLine(83, 10, 92, 19, SSD1306_WHITE);
      }
      if (currentMode == "dance" || currentMode == "party" || currentMode == "excited") {
        star(17, 20);
        star(109, 16);
      }
    }
  }

  display.display();
}

// ==========================================
// 📋 ON-SCREEN OLED MENU SYSTEM
// ==========================================
void renderMenu() {
  display.clearDisplay();

  // Header Banner
  display.fillRect(0, 0, 128, 11, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(14, 2);

  if (currentMenuState == MENU_MAIN) {
    display.print(F("=== MENU SELECT ==="));
    const char* items[] = {
      "1. Gallery Drawings (8)",
      "2. Cute Expressions",
      "3. Preset Messages (5)",
      "4. Settings & Power",
      "5. [ Close Menu ]"
    };
    const uint8_t count = 5;

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
  }
  else if (currentMenuState == MENU_GALLERY) {
    display.print(F("=== GALLERY (8) ==="));
    const char* items[] = {
      "Walking Cat (28F)",
      "Heartface (28F)",
      "Dove with Flower",
      "Sad Face (10F)",
      "Cat & Mouse Cuddle",
      "Girl with Umbrella",
      "Couple High-Five",
      "Girl Portrait",
      "< Back to Menu"
    };
    const uint8_t count = 9;

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
  }
  else if (currentMenuState == MENU_EXPRESSIONS) {
    display.print(F("=== EXPRESSIONS ==="));
    const char* items[] = {
      "Happy Kitty",
      "Love Hearts",
      "Shy Blush",
      "Sweet Sleepy",
      "Laughing",
      "Crying Tear",
      "Angry Pout",
      "Surprised",
      "Starry Wink",
      "Cool Shades",
      "Tiny Kitty",
      "Robot Face",
      "Dancing",
      "Party Star",
      "Calm Hug",
      "< Back to Menu"
    };
    const uint8_t count = 16;

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
  }
  else if (currentMenuState == MENU_MESSAGES) {
    display.print(F("=== MESSAGES ==="));
    const char* items[] = {
      "1. You matter.",
      "2. You make me smile.",
      "3. A little hug for you",
      "4. You are appreciated",
      "5. Keep shining.",
      "< Back to Menu"
    };
    const uint8_t count = 6;

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
  }
  else if (currentMenuState == MENU_SETTINGS) {
    display.print(F("=== SETTINGS ==="));
    String items[4];
    items[0] = String("AutoSleep: [") + (autoSleepEnabled ? "ON" : "OFF") + "]";
    items[1] = String("Wi-Fi AP:  [") + (wifiEnabled ? "ON" : "OFF") + "]";
    items[2] = String("DoubleTap: [") + (doubleTapEnabled ? "ON" : "OFF") + "]";
    items[3] = "< Back to Menu";
    const uint8_t count = 4;

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
// 🔘 HARDWARE BUTTON HANDLERS (BTN1: GPIO 9, BTN2: GPIO 1)
// ==========================================
void handleButtons() {
  bool btn1Reading = digitalRead(BTN1_PIN);
  bool btn2Reading = digitalRead(BTN2_PIN);

  // Debounce Button 1 (Next / Scroll)
  if (btn1Reading != btn1LastState) {
    btn1LastDebounceTime = millis();
  }
  if ((millis() - btn1LastDebounceTime) > DEBOUNCE_DELAY) {
    static bool btn1Processed = false;
    if (btn1Reading == LOW && !btn1Processed) {
      btn1Processed = true;
      lastActivityTime = millis();
      if (isSleeping) { wakeUp(); return; }

      if (currentMenuState == STATE_NORMAL) {
        // Cycle to next mode (CottonKitty 40 modes!)
        modeIdx = (modeIdx + 1) % TOTAL_MODES;
        currentMode = ALL_MODES[modeIdx];
        frameIdx = 0;
        Serial.print(F("[Button 1] Switched to mode: "));
        Serial.println(currentMode);
      } else {
        // Scroll Menu Cursor Down
        uint8_t maxItems = 5;
        if (currentMenuState == MENU_GALLERY) maxItems = 9;
        else if (currentMenuState == MENU_EXPRESSIONS) maxItems = 16;
        else if (currentMenuState == MENU_MESSAGES) maxItems = 6;
        else if (currentMenuState == MENU_SETTINGS) maxItems = 4;

        menuCursor = (menuCursor + 1) % maxItems;
        if (menuCursor >= menuScroll + 4) menuScroll = menuCursor - 3;
        if (menuCursor < menuScroll) menuScroll = menuCursor;
        renderMenu();
      }
    } else if (btn1Reading == HIGH) {
      btn1Processed = false;
    }
  }
  btn1LastState = btn1Reading;

  // Debounce Button 2 (Menu Open / Select)
  if (btn2Reading != btn2LastState) {
    btn2LastDebounceTime = millis();
  }
  if ((millis() - btn2LastDebounceTime) > DEBOUNCE_DELAY) {
    static bool btn2Processed = false;
    if (btn2Reading == LOW && !btn2Processed) {
      btn2Processed = true;
      lastActivityTime = millis();
      if (isSleeping) { wakeUp(); return; }

      if (currentMenuState == STATE_NORMAL) {
        // Open the On-Screen OLED Menu!
        currentMenuState = MENU_MAIN;
        menuCursor = 0;
        menuScroll = 0;
        renderMenu();
      }
      else if (currentMenuState == MENU_MAIN) {
        if (menuCursor == 0) {
          currentMenuState = MENU_GALLERY;
          menuCursor = 0; menuScroll = 0;
        } else if (menuCursor == 1) {
          currentMenuState = MENU_EXPRESSIONS;
          menuCursor = 0; menuScroll = 0;
        } else if (menuCursor == 2) {
          currentMenuState = MENU_MESSAGES;
          menuCursor = 0; menuScroll = 0;
        } else if (menuCursor == 3) {
          currentMenuState = MENU_SETTINGS;
          menuCursor = 0; menuScroll = 0;
        } else if (menuCursor == 4) {
          currentMenuState = STATE_NORMAL; // Close Menu
        }
        if (currentMenuState != STATE_NORMAL) renderMenu();
      }
      else if (currentMenuState == MENU_GALLERY) {
        const char* gModes[] = {
          "gallery_catwalk", "gallery_heartface", "gallery_dove",
          "gallery_sadface", "gallery_catmouse", "gallery_rainygirl",
          "gallery_couple", "gallery_girlportrait"
        };
        if (menuCursor >= 0 && menuCursor < 8) {
          currentMode = gModes[menuCursor];
          frameIdx = 0;
          currentMenuState = STATE_NORMAL;
        } else {
          currentMenuState = MENU_MAIN;
          menuCursor = 0; menuScroll = 0;
          renderMenu();
        }
      }
      else if (currentMenuState == MENU_EXPRESSIONS) {
        const char* eModes[] = {
          "happy", "love", "shy", "sleep", "laugh", "cry",
          "angry", "surprise", "wink", "cool", "tiny",
          "robot", "dance", "party", "calm"
        };
        if (menuCursor >= 0 && menuCursor < 15) {
          currentMode = eModes[menuCursor];
          frameIdx = 0;
          currentMenuState = STATE_NORMAL;
        } else {
          currentMenuState = MENU_MAIN;
          menuCursor = 1; menuScroll = 0;
          renderMenu();
        }
      }
      else if (currentMenuState == MENU_MESSAGES) {
        if (menuCursor >= 0 && menuCursor < 5) {
          greetingIdx = menuCursor;
          currentMessage = GREETINGS[greetingIdx];
          currentMode = "message";
          currentMenuState = STATE_NORMAL;
        } else {
          currentMenuState = MENU_MAIN;
          menuCursor = 2; menuScroll = 0;
          renderMenu();
        }
      }
      else if (currentMenuState == MENU_SETTINGS) {
        if (menuCursor == 0) {
          autoSleepEnabled = !autoSleepEnabled;
          renderMenu();
        } else if (menuCursor == 1) {
          wifiEnabled = !wifiEnabled;
          if (wifiEnabled) { setupWiFi(); }
          else { WiFi.disconnect(true); WiFi.mode(WIFI_OFF); }
          renderMenu();
        } else if (menuCursor == 2) {
          doubleTapEnabled = !doubleTapEnabled;
          renderMenu();
        } else if (menuCursor == 3) {
          currentMenuState = MENU_MAIN;
          menuCursor = 3; menuScroll = 0;
          renderMenu();
        }
      }
    } else if (btn2Reading == HIGH) {
      btn2Processed = false;
    }
  }
  btn2LastState = btn2Reading;
}

// ==========================================
// 💖 TOUCH SENSOR GESTURES (Single Tap, Double Tap & Long Press)
// ==========================================
void handleTouch() {
  bool touch = (digitalRead(TOUCH_PIN) == HIGH);
  unsigned long now = millis();

  if (touch && !touchWasPressed) {
    touchWasPressed = true;
    touchPressStartTime = now;
    lastActivityTime = now;
    if (isSleeping) { wakeUp(); return; }
  }
  else if (!touch && touchWasPressed) {
    touchWasPressed = false;
    unsigned long pressDuration = now - touchPressStartTime;
    lastActivityTime = now;

    if (pressDuration > 1500) {
      currentReaction = REACTION_SNOOZE;
      currentMode = "sleepy";
      reactionStartTime = now;
      Serial.println(F("[Touch] Long press detected -> Snooze mode"));
    } else {
      if (doubleTapEnabled && (now - lastTouchReleaseTime < 380)) {
        currentReaction = REACTION_BURST;
        reactionStartTime = now;
        lastTouchReleaseTime = 0;
        Serial.println(F("[Touch] Double-tap detected -> Heart Explosion Burst!"));
      } else {
        lastTouchReleaseTime = now;
        currentReaction = REACTION_LOVE;
        reactionStartTime = now;
        currentMode = "love";
        Serial.println(F("[Touch] Single tap detected -> Love reaction"));
      }
    }
  }
}

// ==========================================
// ⚡ POWER MANAGEMENT (Auto-Sleep & Instant Wake)
// ==========================================
void goToSleep() {
  isSleeping = true;
  display.clearDisplay();
  display.display();
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  Serial.println(F("[Power] OLED Auto-Sleep active to conserve battery."));
}

void wakeUp() {
  isSleeping = false;
  display.ssd1306_command(SSD1306_DISPLAYON);
  lastActivityTime = millis();
  Serial.println(F("[Power] Woke up from sleep!"));
}

// ==========================================
// 🔌 USB-C OTG SERIAL PROTOCOL
// ==========================================
void handleSerial() {
  while (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    lastActivityTime = millis();
    if (isSleeping) wakeUp();

    if (line.startsWith("mode:")) {
      currentMode = line.substring(5);
      frameIdx = 0;
      Serial.println(F("OK:MODE_SET"));
    }
    else if (line.startsWith("face:")) {
      String param = line.substring(5);
      if (param.toInt() >= 0 && param.toInt() < TOTAL_MODES && (param != "0" || line == "face:0")) {
        modeIdx = param.toInt();
        currentMode = ALL_MODES[modeIdx];
      } else {
        currentMode = param;
      }
      frameIdx = 0;
      Serial.println(F("OK:FACE_SET"));
    }
    else if (line.startsWith("msg:")) {
      currentMessage = line.substring(4);
      currentMode = "message";
      Serial.println(F("OK:MSG_SET"));
    }
    else if (line == "pat") {
      currentReaction = REACTION_LOVE;
      reactionStartTime = millis();
      currentMode = "love";
      Serial.println(F("OK:PAT"));
    }
    else if (line == "doubletap") {
      currentReaction = REACTION_BURST;
      reactionStartTime = millis();
      Serial.println(F("OK:DOUBLETAP"));
    }
    else if (line == "status") {
      Serial.print(F("STATUS:name=CottonKitty,mode="));
      Serial.print(currentMode);
      Serial.print(F(",ip="));
      Serial.print(wifiIP);
      Serial.print(F(",wifi="));
      Serial.println(wifiEnabled ? "on" : "off");
    }
    else {
      Serial.println(F("ERR:UNKNOWN_CMD"));
    }
  }
}

// ==========================================
// 📶 WI-FI & WEB SERVER ROUTES
// ==========================================
void sendApp() {
  server.send_P(200, "text/html; charset=utf-8", APP_HTML);
}

void sendStatus() {
  String json = String(R"({"name":"CottonKitty","connected":true,"mode":")") + currentMode + String(R"(","ip":")") + wifiIP + String(R"("})");
  server.send(200, "application/json", json);
}

void modeRoute() {
  if (server.hasArg("name")) {
    currentMode = server.arg("name");
    frameIdx = 0;
    lastActivityTime = millis();
    if (isSleeping) wakeUp();
  }
  server.send(200, "application/json", R"({"ok":true})");
}

void messageRoute() {
  if (server.hasArg("text")) {
    currentMessage = server.arg("text");
    currentMode = "message";
    lastActivityTime = millis();
    if (isSleeping) wakeUp();
  }
  server.send(200, "application/json", R"({"ok":true})");
}
void setupWiFi() {
  WiFi.mode(WIFI_STA);
  Serial.print(F("[WiFi] Connecting to: "));
  Serial.println(HOME_WIFI_SSID);

  WiFi.begin(HOME_WIFI_SSID, HOME_WIFI_PASS);
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 7000) {
    delay(200);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiIP = WiFi.localIP().toString();
    Serial.println(); Serial.print(F("[WiFi] Connected to Home Wi-Fi! IP: "));
    Serial.println(wifiIP);
  } else {
    Serial.println(); Serial.println(F("[WiFi] Starting SoftAP CottonKitty (192.168.4.1)... "));
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    wifiIP = WiFi.softAPIP().toString();
    Serial.print(F("[WiFi] AP IP: "));
    Serial.println(wifiIP);
  }

  server.on("/", sendApp);
  server.on("/api/status", sendStatus);
  server.on("/api/mode", modeRoute);
  server.on("/api/message", messageRoute);
  server.begin();
  Serial.println(F("[WiFi] HTTP WebServer started on port 80."));
}

// ==========================================
// 🚀 SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println(); Serial.println(F("============================================"));
  Serial.println(F("🐾 CottonKitty & Little Reminder Unified FW 🐾"));
  Serial.println(F("============================================"));

  // Pins Setup
  pinMode(BTN1_PIN, INPUT_PULLUP);
  pinMode(BTN2_PIN, INPUT_PULLUP);
  pinMode(TOUCH_PIN, INPUT_PULLDOWN);

  // OLED Initialization
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("[Error] SSD1306 OLED init failed! Check SDA=GPIO5, SCL=GPIO6"));
    while (true) delay(1000);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(18, 12);
  display.println(F("CottonKitty &"));
  display.setCursor(18, 24);
  display.println(F("Little Reminder"));
  display.setCursor(18, 42);
  display.println(F("Starting..."));
  heart(108, 18, 3);
  star(10, 18);
  display.display();
  delay(1200);

  // Setup Wi-Fi and Web Server
  setupWiFi();

  lastActivityTime = millis();
  Serial.println(F("[Setup] Initialization complete! Enjoy your companion."));
}

// ==========================================
// 🔄 MAIN LOOP
// ==========================================
void loop() {
  // 1. Handle Wi-Fi Client Requests
  if (wifiEnabled) {
    server.handleClient();
  }

  // 2. Handle USB-C OTG Serial Commands
  handleSerial();

  // 3. Handle Hardware Buttons (BTN1: GPIO 9, BTN2: GPIO 1)
  handleButtons();

  // 4. Handle Touch Sensor Gestures
  handleTouch();

  // 5. Auto-Sleep Battery Management
  if (autoSleepEnabled && !isSleeping) {
    if (millis() - lastActivityTime > AUTO_SLEEP_TIMEOUT) {
      goToSleep();
    }
  }

  // 6. OLED Frame Render
  if (!isSleeping) {
    if (currentMenuState == STATE_NORMAL) {
      drawMode();
    } else {
      renderMenu();
    }
  }

  delay(12);
}
