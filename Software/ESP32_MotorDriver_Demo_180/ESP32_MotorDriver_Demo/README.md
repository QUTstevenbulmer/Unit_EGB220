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
WiFi: motordriver-ab12
Pass: motors123
http://192.168.4.1
```

Every board has a unique name, `motordriver-` + the last 4 digits of its MAC address. You can rename it (see *Robot name* below).

Join it from a phone. A **"Sign in to network"** pop-up should open the control page by itself. If it doesn't, browse to **http://192.168.4.1**. Android may warn that the network has no internet; choose **"Stay connected"**.

### Putting the robot on your own WiFi

The robot can remember up to **5 networks** (for example QUT, home and a phone hotspot). Each time it powers up it scans, then tries the saved networks that are in range **from the top of the list down**. If none of them work, it makes its own hotspot, so you can always reach it.

1. On the control page, scroll to **WiFi setup** (the blue banner at the top links to it).
2. Tap **Scan for networks** and pick yours, or type the name.
   **Security** is filled in automatically for scanned networks:
   - **Personal**: one WiFi password (home routers, phone hotspots).
   - **Enterprise**: your username and password (eduroam, uni/work WiFi). The method is usually **PEAP**. Leave *anonymous identity* blank unless your IT department says otherwise.
3. Tap **Save network**. Repeat for each network.
   - In hotspot mode the robot restarts straight away to connect.
   - When it's already on a network, new ones are just saved for next time. Tap **Restart & connect** to try now.
4. The robot joins the first network it can. The OLED shows its new address.
5. Put your phone back on the same WiFi and browse to that address, or **http://*robot-name*.local** (e.g. http://team3.local).
   `.local` names work on iPhone, Mac and Windows, but not on some Android phones; use the IP there.

In the saved list, tap **↑** to make a network be tried first, or **×** to remove it. Networks are saved in flash and survive re-uploading code. Passwords are never sent back to the page. From the serial monitor, use `wifi set`, `wifi list`, `wifi top <n>`, `wifi remove <n>`, `wifi scan`, `wifi forget` or `wifi`.

**Back to hotspot mode:** tap **Forget all** on the page, type `wifi forget`, or **hold SW1 while powering on**. All three forget every saved network.
If none of the saved networks can be joined, the robot falls back to its hotspot automatically, and the page shows why.
If the network it joined disappears for 60 s while the robot is idle (`WIFI_LOST_RESTART_S`), it restarts and looks again.

Networks that hide their name (hidden SSIDs) are only tried if the scan finds nothing at all.

> **Limits:**
> - The ESP32-S2 only supports **2.4 GHz** WiFi.
> - Networks with a **web sign-in page** (hotel or guest WiFi) won't work.
> - **Enterprise networks:** the robot doesn't check the network's server certificate. That's normal for hobby boards, but it means the robot would also log in to a fake access point with the same name. Use a dedicated or low-value account if you can.
> - Many campus networks also **block devices from talking to each other** (client isolation) and block `.local` names. The robot can join fine while phones still can't open its page. Test with one board first. If it's blocked, use the robot's own hotspot or a phone hotspot in class.

### Robot password (stops other teams driving your robot)

Each robot has **one password**, used for:

- **logging in to the web pages** – asked once per browser session, and again after the robot restarts
- **the robot's own hotspot** WiFi password
- **firmware updates** – web upload, and PlatformIO / Arduino IDE network uploads

Every robot starts with **`motors123`** (`ROBOT_PASSWORD` in `config.h`). Each team should set its own. On the Test page, go to **Robot setup → Robot password** (it needs the current password), or type `password <new>` in the serial monitor. It must be 8+ characters, because WiFi requires that. The robot restarts. Then log in again, and on the hotspot reconnect with the new password.

Forgotten it? **Hold SW2 while powering on**, or type `password default` over USB. Either sets it back to `motors123`. The OLED only shows the hotspot password while it's still the default.

> This keeps classmates out; it isn't bank-grade security. The pages are plain HTTP, so someone capturing WiFi traffic could see the password. Set `WEB_LOGIN_REQUIRED` to `false` in `config.h` to turn the login off.

### Robot name (hostname)

The robot's name is used for **both** its hotspot name and its web address **http://*name*.local**, so students can find their robot without knowing its IP address. Give each robot its own name, for example `team3`:

- On the page: **Robot setup → Robot name → Save name**, or
- From the serial monitor: `name team3` (`name` on its own shows the current name; `name default` goes back to `motordriver-xxxx`).

Names may only use lowercase `a-z`, `0-9` and `-` (no spaces, max 24 characters). The robot reboots to use the new name. It's kept in flash, even after re-uploading code.

### Check motors & encoders (new robots)

On the Test page, **Robot setup → Check motors & encoders** walks through each wheel with the wheels off the ground:

1. It spins the wheel "forward" for 1 s and asks which way it turned. If you answer *Backward*, it reverses that motor and spins it again.
2. Once the motor is right, it checks the encoder counted **up**. If it counted down, the encoder is reversed automatically. If it barely counted, it tells you to check the wiring.
3. Finally it offers to **measure top speed** for speed control.

### Speed control (PID) – driving straight

Without feedback, "50 %" means 50 % of the battery's power. Two motors never quite match, so the robot curves. With **speed control** on, 50 % means half of **top speed**. Every 20 ms, each wheel's real speed is measured from its encoder and a PID loop adjusts the power to match. While both wheels have the same target, a second term (**Ks**) compares how far each wheel has turned and trims them back into step.

In simulation with one motor 20 % stronger, the robot without speed control drifted to a 1,300-count difference between the wheels after 4 s, about half a wheel turn. With speed control the wheels stayed within about 20 counts.

Setting up (once per robot, saved in flash):

1. Run **Check motors & encoders** above, so both motors and encoders count the right way. **Speed control with a backwards encoder pushes the motor flat out.** A safety check catches this and switches PID off, but fix the direction first.
2. Wheels off the ground: **Measure top speed** on the Test page, or `pid cal`.
3. Tick **Use the encoders to control speed** (Test page), tap **Speed control** on the Drive page, or type `pid on`.

Everything that drives the motors (sliders, joystick, `d 50 50`, demo, line follow) then uses speed control.

Tuning: `pid log` (or **Show live values in Terminal**) prints target / measured / power 10× a second. Change gains on the page or with `pid kp 0.8`, `pid ki 4`, `pid kd 0`, `pid ks 8`.

| Symptom | Try |
|---|---|
| Wobbles / oscillates | lower `kp`, then `ki` |
| Slow to reach speed | raise `kp` |
| Settles slightly off target | raise `ki` |
| Still drifts off straight | raise `ks` |
| "[pid] Motor A: power … but encoder reads …" | encoder unplugged, backwards (`rev enca`) or wheel stalled. PID switches itself off. |

### Reversing motors, encoders and servos

If a motor spins backwards (wires swapped, or the motors face opposite ways on a two-wheel chassis), there's no need to rewire it. Tick **Reverse motor A/B** in the page's **Robot setup** card, or type `rev a` / `rev b`. Do the same for encoders that count the wrong way (`rev enca`, `rev encb`) and servos mounted the other way round (`rev s1`, `rev s2`, which swaps 0° and 180°). Type `rev` to see what's reversed.

These are saved in flash. The `*_INVERTED` values in `config.h` are only the defaults for a brand-new board. Fix the motor direction first, then the encoder.

### Two web pages

| Address | Page | For |
|---|---|---|
| `http://<robot>/` | **Test & debug** | Every device: motors, encoders, line sensor, servos, LEDs, settings, WiFi, firmware update |
| `http://<robot>/drive` | **Drive** | Driving round a track. The line sensor and joystick are at the top, so both fit on a phone screen; STOP / Brake / Line follow and motor/encoder readouts sit underneath |

Each page links to the other. Both have a **Terminal** (see below).

**Drive page joystick:** drag up for forward, down for reverse, left/right to turn. Let go and the robot stops. On a laptop, use the arrow keys or WASD, and Space for stop. **Max speed** limits the joystick. It assumes motor A is the left wheel. If turning is backwards, tick *Motor A is the right wheel*. Both settings are remembered by that browser. Fix wheels spinning the wrong way first with **Reverse motor** on the Test page.

### Web terminal (debug over WiFi)

Everything the firmware prints to the USB serial monitor also appears in the **Terminal** on both pages. You can type the same commands there (`help`, `info`, `d 30 30`, `wifi list` ...). The robot keeps the last 6 kB (`LOG_BUFFER_BYTES`), so you can open the page after something happened and still see it.

To send your own debug messages there, print with **`Log`** instead of `Serial`. It works the same way and goes to both USB and the web:

```cpp
Log.printf("Left encoder: %ld\r\n", encoderCount(0));
Log.println("Turning left");
```

Commands that wait for key presses (`enc`, `line`, `buttons`, `wifi set`, `selftest`, `blink`) only work over USB. The pages already show those values live.

### The Test & debug page

It has motor sliders, STOP/brake, demo, line follow, live encoders, a line-sensor view, servo sliders, LED buttons, a ramp setting, robot setup and WiFi setup.
Every card has a **"? What does this do?"** link that explains the controls for students. **Open all** at the top expands them all.
For safety, the motors stop by themselves if the page stops sending for 1 s.

### OTA updates (new code over WiFi)

After the first USB flash, new code can be installed over WiFi. Everything stops, the new code is written into the spare app slot and checked, then the robot restarts. If the upload fails, the old code keeps running. Saved WiFi, robot name and reverse settings are kept.

- **Web page:** in the **Firmware update** card, pick the `.bin` and enter the update password.
  - PlatformIO: **Build**, then use `.pio/build/motordriver/firmware.bin`.
  - Arduino IDE: **Sketch → Export Compiled Binary**. Use the plain `.bin`, not the `bootloader`, `partitions` or `merged` one.
- **PlatformIO direct:** edit `upload_port` in the `[env:ota]` section of `platformio.ini` to your robot's name (e.g. `team3.local`) or IP, then upload with the `ota` environment (`pio run -e ota -t upload`).
- **Arduino IDE:** the robot appears under **Tools → Port** as a network port with its name.

Network uploads and the web upload both use the **robot password** (see below). In `platformio.ini`, set `--auth` to your robot password. USB always works as a fallback, including the BOOT + RESET routine if a bad upload stops the code from running.

> PlatformIO/Arduino network uploads need your computer and the robot on the same network, with `.local` names working (or use the IP). Campus networks that block devices talking to each other will block this too. The web-page upload works anywhere you can open the page.

### Web API (for students' own apps / Python)

Log in first. The robot gives back a session cookie, so use a `requests.Session`:

```python
import requests
s = requests.Session()
s.post("http://192.168.4.1/login", data={"pw": "motors123"})   # your robot password
s.get("http://192.168.4.1/api/drive", params={"a": 40, "b": 40})
print(s.get("http://192.168.4.1/api/status").json())
```

```
POST /login                   pw=<robot password> -> session cookie   (GET /logout)
POST /api/password            current=<old>, pw=<new> ("" = default) -> restarts
GET /drive                    the Drive page (/ is the Test & debug page)
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
POST /api/wifi/save           ssid, pass [, type=enterprise, user, ident, eap] -> adds to the list
GET /api/wifi/list            saved networks in try order (no passwords)
GET /api/wifi/remove?i=0      remove one        GET /api/wifi/top?i=2   try it first
GET /api/wifi/restart         restart and look for saved networks
GET /api/wifi/forget          forget all networks, back to hotspot mode (reboots)
GET /api/config               JSON: robot name + reverse settings
GET /api/config?motorA=1      reverse options: motorA motorB encA encB servo1 servo2 (0/1, saved)
POST /api/name                form field name -> renames and reboots ("" = default name)
POST /api/update              multipart upload of firmware.bin -> installs and reboots
GET /api/pid                  speed control settings  (?on=1, ?kp=0.8&ki=4..., ?cal=1 = measure top speed)
GET /api/setup/spin?m=0       spin motor A (m=1 B) forward 1 s -> {"counts":N}
GET /api/log?from=N           terminal text since N (header X-Next = new N; from=-1 = all kept)
POST /api/cmd                 form field cmd = any serial command, e.g. cmd=d 30 30
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
WiFi        wifi           wifi scan        wifi set        wifi list
            wifi top <n>   wifi remove <n>  wifi forget     reboot
Speed (PID) pid            pid on | off     pid cal         pid log
            pid kp|ki|kd|ks <value>
Setup       name           name team3       name default
            password <new> password default (also: hold SW2 at power-up)
            rev            rev a | b | enca | encb | s1 | s2   (toggles, saved)
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
| `MOTOR_A_INVERTED` / `MOTOR_B_INVERTED` | Default direction for a new board. Normally change it at run time instead (`rev a`, or the web page). |
| `ENC_COUNTS_PER_REV` | Pulses × 4 × gear ratio (e.g. 7 × 4 × 100 = 2800). With 0, speed shows as counts/s. |
| `ENC_A_INVERTED` / `ENC_B_INVERTED` | Defaults. `selftest` tells you if an encoder counts backwards; fix it with `rev enca` / `rev encb`. |
| `LINE_ACTIVE_HIGH` | Flip if your sensor bar reads "line" on white instead of black. |
| `FOLLOW_BASE_SPEED`, `FOLLOW_KP` | Line-follow tuning. If the robot steers *away* from the line, make `FOLLOW_KP` negative. |
| `SERVO_MIN_US` / `SERVO_MAX_US` | Pulse range of your servos. |
| `SERVO_1_INVERTED` / `SERVO_2_INVERTED` | Defaults; change at run time with `rev s1` / `rev s2`. |
| `DEFAULT_HOSTNAME` | Start of the default robot name (`motordriver-xxxx`). |
| `ROBOT_PASSWORD` | Default robot password (web login, hotspot, OTA). Teams set their own at run time. |
| `WEB_LOGIN_REQUIRED` | `false` = no login on the web pages. |
| `WIFI_STA_*` | Optional default network to join. |

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
| A motor goes backwards | Tick *Reverse motor* on the page, or `rev a` / `rev b`. |
| `name.local` doesn't open | Some Android phones and campus networks don't support `.local` names. Use the IP on the OLED. |
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
src/WebUI.cpp               WiFi, web server, OTA
src/WebPage.h               Test & debug page (/)
src/DrivePage.h             Drive page (/drive)
src/TermJs.h                web terminal script, shared by both pages
src/Log.cpp, include/Log.h  Log.printf -> USB serial + web terminal
src/SelfTest.cpp            guided hardware test
src/Leds.cpp                status LEDs
src/Buttons.cpp             debounced buttons (short / long press)
src/Servos.cpp              servo PWM
src/Encoders.cpp            interrupt-driven quadrature decoding
src/LineSensor.cpp          line sensor + line-follow P controller
src/Display.cpp             OLED pages
src/HeaderIO.cpp            pin map, raw pin tests, I2C scan
src/Settings.cpp            robot name + reverse options (saved in flash)
src/SpeedControl.cpp        encoder speed control (PID) + drive straight
```

### Student exercise ideas

- Add a new move to `DEMO_STEPS` in `main.cpp`.
- Turn the line follower into a **PD** controller (add a term for how fast `pos` changes).
- Use the encoders to drive **exactly 1 m**, or to keep both wheels at the same speed.
- Make a servo "radar" sweep and show its angle on the OLED.
- Write a Python script that calls `/api/drive` to drive a square.
- Turn speed control off and on and compare how straight the robot drives 2 m. Then tune `kp` / `ki` with `pid log` running.
- Use the encoder counts to drive exactly 1 m, or turn exactly 90°.
- Add `Log.printf` lines to the line follower and watch its decisions in the web terminal while it runs.
- Add a new OLED page in `Display.cpp`.
