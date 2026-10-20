# ESP32-S2 Motor Driver + Breakout Board – Demo & Test Firmware

Arduino firmware (PlatformIO) for the **ESP32-S2-SOLO-N4 + TB6612FNG controller** plugged into the **breakout board (BOB)**.

It exercises every feature: two encoder motors, two servos, an 8-channel line sensor, the OLED, four LEDs, three buttons and WiFi. It also has a guided **self test** for bringing up new boards.

---

## 1. Getting started

1. Install **VS Code** and the **PlatformIO IDE** extension.
2. *File → Open Folder…* and pick this folder (the one with `platformio.ini`).
   On the first build, PlatformIO downloads the ESP32 tools and the OLED libraries. This takes a few minutes.
3. Plug the controller in with a **data** USB-C cable.

### First flash (once per new board)

A blank board has no USB code yet, so put it into download mode by hand:

1. **Hold BOOT**, tap **RESET (EN)**, then release BOOT.
2. Click **Upload** (→) in the blue PlatformIO status bar.
3. When it says *SUCCESS*, **tap RESET** to start the program.

After that, just click **Upload**. If an upload ever fails, repeat the BOOT + RESET routine.

### Serial monitor

Click the 🔌 icon in the status bar. A banner and the command list appear when it connects. Type `help` and press Enter at any time.

---

## 2. Using it without a computer

| Control | Action |
|---|---|
| **SW1** (BOB) | Start / stop the motor + servo "dance" demo |
| **SW2** (BOB) | Start / stop **line following** |
| **BOOT** (controller) | Next OLED page |
| **Hold SW1 while powering on** | Forget saved WiFi, start in hotspot mode |
| **Hold any button 1 s** | **Emergency stop**: motors brake, driver goes to standby |

**Status LEDs** (auto mode):

| LED | Meaning |
|---|---|
| Green | Breathing = program running |
| Blue | Solid = a phone is connected; blinking = the web page is driving |
| Yellow | Demo or line-follow running |
| Red | Motors are powered and moving |

**OLED pages** (BOOT flips between them):

1. **WiFi:** network name, hotspot password, address, mode
2. **Motors:** speed %, encoder counts and speed
3. **Line sensor:** live picture of the 8 sensors
4. **Inputs:** buttons, servo angles, chip temperature

---

## 3. WiFi and phone control

### First time: the robot's own hotspot

With no network saved, the robot makes its own WiFi hotspot. **OLED page 1 shows its name, password and address**:

```
WiFi: MotorDriver-AB12
Pass: motors123
http://192.168.4.1
```

Join it from a phone. A **"Sign in to network"** pop-up should open the control page by itself. If it doesn't, browse to **http://192.168.4.1**. Android may warn that the network has no internet; choose **"Stay connected"**.

### Putting the robot on your own WiFi

1. On the control page, scroll to **WiFi setup** (the blue banner at the top links to it).
2. Tap **Scan for networks** and pick yours, or type the name.
   **Security** is filled in automatically for scanned networks:
   - **Personal**: one WiFi password (home routers, phone hotspots).
   - **Enterprise**: your username and password (eduroam, uni/work WiFi). The method is usually **PEAP**. Leave *anonymous identity* blank unless your IT department says otherwise.
3. Tap **Save & join**.
4. The robot reboots and joins your network. The OLED shows its new address.
5. Put your phone back on the same WiFi and browse to that address, or **http://motordriver.local**.
   `.local` names work on iPhone, Mac and Windows, but not on some Android phones; use the IP there.

The network is saved in flash and survives re-uploading code. From the serial monitor, use `wifi set`, `wifi scan`, `wifi forget` or `wifi`.

**Back to hotspot mode:** tap **Use hotspot** on the page, type `wifi forget`, or **hold SW1 while powering on**.
If the saved network can't be found, the robot falls back to its hotspot automatically, and the page shows why.

> **Limits:**
> - The ESP32-S2 only supports **2.4 GHz** WiFi.
> - Networks with a **web sign-in page** (hotel or guest WiFi) won't work.
> - **Enterprise networks:** the robot doesn't check the network's server certificate. That's normal for hobby boards, but it means the robot would also log in to a fake access point with the same name. Use a dedicated or low-value account if you can.
> - Many campus networks also **block devices from talking to each other** (client isolation) and block `.local` names. The robot can join fine while phones still can't open its page. Test with one board first. If it's blocked, use the robot's own hotspot or a phone hotspot in class.

### The control page

It has motor sliders, STOP/brake, demo, line follow, live encoders, a line-sensor view, servo sliders, LED buttons, a ramp setting and WiFi setup.
For safety, the motors stop by themselves if the page stops sending for 1 s.

### Web API (for students' own apps / Python)

```
GET /api/drive?a=50&b=-50     motor speeds -100..100 (repeat at least every 1 s)
GET /api/stop                 stop everything        (/api/stop?brake=1 to brake)
GET /api/demo                 toggle demo
GET /api/follow               toggle line following
GET /api/servo?n=1&deg=90     servo 1/2 to 0..180    (deg=-1 = off)
GET /api/led?c=r&v=255        LED r/g/b/y 0..255     (c=auto / c=off)
GET /api/ramp?ms=600          ramp time
GET /api/enc/reset            zero encoder counts
GET /api/status               JSON with everything above (incl. WiFi)
GET /api/wifi/scan            scan for networks (call again until "scanning":false)
POST /api/wifi/save           form fields ssid, pass -> saves and reboots
GET /api/wifi/forget          back to hotspot mode (reboots)
```

---

## 4. Serial commands

```
Motors      a <spd>   b <spd>   d <a> <b>   stop   brake   stby <0|1>   ramp <ms>   demo
Encoders    enc            live counts + speed      enc reset
Line        line           live sensor view         follow
Servos      servo 1 90     servo 2 off              sweep 1
LEDs        led r 255      led auto                 led off
Inputs      buttons        oled (next page)         i2c (scan)
WiFi        wifi           wifi scan        wifi set        wifi forget
Board       pins   blink <gpio>   read   info   selftest   help
```

---

## 5. Connectors and pin map

### Breakout board

| Connector | Pins | Use |
|---|---|---|
| **J7 XT30** | | Battery in (2S–3S). Goes through Q1 + F1 to *Vbatt* |
| **J5 JST-XH** | 1 M+ · 2 M− · 3 3V3 · 4 enc B · 5 enc A · 6 GND | Motor A with encoder |
| **J6 JST-XH** | same as J5 | Motor B with encoder |
| **J8 / J9** | GND · +5V · signal | Servo 1 (IO1) / Servo 2 (IO2) |
| **J15 2×8** | odd pins = LF_1…LF_8, even pins = GND | Line sensor (digital) |
| **J14 2×3** | GND · +5V · RXD · TXD · SDA_5V · SCL_5V | 5 V I2C (via PCA9306) + UART |
| **U2** | I2C 0x3C | 0.91" 128×32 OLED |

### GPIO map

| Function | GPIO | | Function | GPIO |
|---|---|---|---|---|
| Motor A PWM / IN1 / IN2 | 10 / 21 / 14 | | LED red / green / blue / yellow | 4 / 5 / 6 / 7 |
| Motor B PWM / IN1 / IN2 | 38 / 36 / 37 | | SW1 / SW2 / BOOT | 8 / 13 / 0 |
| Motor STBY | 35 | | Servo 1 / 2 | 1 / 2 |
| Encoder A: ch A / ch B | 9 / 3 | | Line LF_1 … LF_4 | 42, 41, 40, 39 |
| Encoder B: ch A / ch B | 12 / 11 | | Line LF_5 … LF_8 | 18, 17, 16, 15 |
| I2C SDA / SCL | 34 / 33 | | UART TX / RX | 43 / 44 |

Every GPIO is now used, so add more I/O with I2C modules on J14.

---

## 6. Settings to check (`include/config.h`)

| Setting | Why |
|---|---|
| `MOTOR_A_INVERTED` / `MOTOR_B_INVERTED` | On a two-wheel robot the motors face opposite ways. Flip one so `d 50 50` drives forward. |
| `ENC_COUNTS_PER_REV` | Pulses × 4 × gear ratio (e.g. 7 × 4 × 100 = 2800). With 0, speed shows as counts/s. |
| `ENC_A_INVERTED` / `ENC_B_INVERTED` | `selftest` tells you if an encoder counts backwards. |
| `LINE_ACTIVE_HIGH` | Flip if your sensor bar reads "line" on white instead of black. |
| `FOLLOW_BASE_SPEED`, `FOLLOW_KP` | Line-follow tuning. If the robot steers *away* from the line, make `FOLLOW_KP` negative. |
| `SERVO_MIN_US` / `SERVO_MAX_US` | Pulse range of your servos. |
| `WIFI_*` | Network name and password, or join an existing network. |

---

## 7. Bring-up checklist (new boards)

1. **Before power:** check for shorts between 3V3–GND, 5V–GND and Vbatt–GND with a meter.
2. **Controller alone on USB:** it should enumerate. Measure 3.2–3.4 V on J7-10.
3. **First flash**, as above.
4. **Plug into the BOB, still on USB:** the OLED should show the WiFi name.
5. **Battery:** use a current-limited bench supply first (8 V, 0.5 A limit) on the XT30.
6. **Wheels off the table**, then run `selftest`.

### Troubleshooting

| Symptom | Try |
|---|---|
| No COM port | Use a data USB-C cable. Do the BOOT + RESET routine. |
| No serial output after upload | Tap RESET. Close and reopen the monitor. |
| OLED blank / "NOT found" | It needs the battery or 5 V (it runs from the BOB's 5 V). Run `i2c` and look for 0x3C. |
| Motors don't move | Battery on the XT30? Run `info` to check for standby. |
| Can't find the robot after saving WiFi | Check the OLED for its IP. Is your phone on the same network? Is it 2.4 GHz? Hold SW1 at power-up to start over. |
| Encoder shows 0 | Check the JST wiring and that the encoder runs on 3.3 V. |
| Servos jitter or the board resets | Servos are pulling the 5 V regulator down. See hardware notes. |
| Line sensor always "none" | J15 has no power pin; the sensor bar needs 3V3 from elsewhere. Check `LINE_ACTIVE_HIGH`. |

---

## 8. Hardware notes (BOB v01)

- **Servo power:** +5V comes from an LDL1117 linear regulator fed from the battery. It handles one or two *micro* servos at 2S. Larger servos, or 3S packs, will overheat it or cause brown-outs.
- **Line-sensor header J15** carries only signals and GND. Power the sensor bar from **3.3 V** (J14 has only 5 V). The ESP32 inputs are **not 5 V tolerant**.
- **Reverse-battery MOSFET Q1 on the BOB** is fitted source-to-battery, so it won't block a reversed pack. The controller's own protection still guards the controller. Always check XT30 polarity.

---

## 9. Project layout

```
platformio.ini              build settings + libraries (Adafruit SSD1306 / GFX)
boards/                     ESP32-S2-SOLO-N4 board definition (4 MB flash, no PSRAM)
include/pins.h              every GPIO, from both schematics
include/config.h            settings to change
include/PwmCompat.h         PWM helpers + LEDC channel plan
lib/TB6612/                 reusable TB6612FNG motor library
src/main.cpp                setup / loop, modes, demo, buttons
src/SerialMenu.cpp          text commands
src/WebUI.cpp, WebPage.h    WiFi, web server, phone page
src/SelfTest.cpp            guided hardware test
src/Leds.cpp                status LEDs
src/Buttons.cpp             debounced buttons (short / long press)
src/Servos.cpp              servo PWM
src/Encoders.cpp            interrupt-driven quadrature decoding
src/LineSensor.cpp          line sensor + line-follow P controller
src/Display.cpp             OLED pages
src/HeaderIO.cpp            pin map, raw pin tests, I2C scan
```

### Student exercise ideas

- Add a new move to `DEMO_STEPS` in `main.cpp`.
- Turn the line follower into a **PD** controller (add a term for how fast `pos` changes).
- Use the encoders to drive **exactly 1 m**, or to keep both wheels at the same speed.
- Make a servo "radar" sweep and show its angle on the OLED.
- Write a Python script that calls `/api/drive` to drive a square.
- Add a new OLED page in `Display.cpp`.
