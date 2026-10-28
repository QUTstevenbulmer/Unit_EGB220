// =============================================================================
//  WebPage.h  -  The phone/laptop control page (plain HTML + JavaScript).
//  Stored in flash and sent by WebUI.cpp when you browse to the board.
// =============================================================================
#pragma once
#include <Arduino.h>

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Motor Driver</title>
<style>
  :root { --bg:#f4f5f7; --card:#fff; --ink:#1d2330; --mute:#6b7383; --acc:#2563eb; --stop:#dc2626; --on:#16a34a; }
  @media (prefers-color-scheme: dark) {
    :root { --bg:#12151b; --card:#1c212b; --ink:#e8ebf1; --mute:#98a1b3; --acc:#60a5fa; --stop:#f87171; --on:#4ade80; }
  }
  * { box-sizing:border-box; }
  body { margin:0; font-family:system-ui,sans-serif; background:var(--bg); color:var(--ink); }
  main { max-width:520px; margin:0 auto; padding:16px; }
  h1 { font-size:1.3rem; margin:4px 0 12px; }
  h2 { font-size:.8rem; text-transform:uppercase; letter-spacing:.06em; color:var(--mute); margin:0 0 8px; }
  .card { background:var(--card); border-radius:14px; padding:16px; margin-bottom:14px;
          box-shadow:0 1px 3px rgba(0,0,0,.08); }
  .row { display:flex; justify-content:space-between; align-items:baseline; gap:8px; }
  .val { font-variant-numeric:tabular-nums; font-weight:600; }
  input[type=range] { width:100%; height:40px; accent-color:var(--acc); }
  .btns { display:grid; grid-template-columns:1fr 1fr; gap:10px; }
  .btns4 { display:grid; grid-template-columns:repeat(4,1fr); gap:8px; }
  button { font-size:1rem; padding:12px; border:0; border-radius:10px; cursor:pointer;
           background:var(--bg); color:var(--ink); font-weight:600; }
  button.stop { background:var(--stop); color:#fff; grid-column:span 2; font-size:1.2rem; padding:16px; }
  button.on { background:var(--acc); color:#fff; }
  .led { width:100%; }
  .lr.on{background:#ef4444} .lg.on{background:#22c55e} .lb.on{background:#3b82f6} .ly.on{background:#eab308;color:#111}
  .line { display:grid; grid-template-columns:repeat(8,1fr); gap:6px; margin-top:6px; }
  .line div { height:26px; border-radius:6px; border:2px solid var(--mute); }
  .line div.hit { background:var(--ink); border-color:var(--ink); }
  .status { color:var(--mute); font-size:.9rem; line-height:1.6; }
  label { color:var(--mute); font-size:.9rem; }
  .dot { display:inline-block; width:10px; height:10px; border-radius:50%; background:var(--mute); margin-right:4px; }
  .dot.on { background:var(--on); }
  .banner { background:var(--acc); color:#fff; border-radius:14px; padding:14px 16px; margin-bottom:14px; line-height:1.45; }
  .banner a { color:#fff; font-weight:700; }
  input[type=text], input[type=password], select { width:100%; font-size:1rem; padding:12px; margin:4px 0 10px;
           border-radius:10px; border:1px solid var(--mute); background:var(--bg); color:var(--ink); }
  .msg { font-size:.9rem; margin-top:8px; min-height:1.2em; }
  .err { color:var(--stop); }
  .ok { color:var(--on); }
  #termOut { height:220px; overflow:auto; margin:0; padding:8px; border-radius:8px; background:#0b0f16; color:#d1fae5;
             font:12px/1.4 ui-monospace,Menlo,Consolas,monospace; white-space:pre-wrap; word-break:break-all; }
  #termIn { width:100%; font:14px ui-monospace,Menlo,Consolas,monospace; padding:10px; margin-top:8px; border-radius:8px;
            border:1px solid var(--mute); background:var(--bg); color:var(--ink); }
  details.help { margin-top:10px; font-size:.88rem; line-height:1.5; color:var(--mute); }
  details.help summary { cursor:pointer; color:var(--acc); font-weight:600; list-style:none; }
  details.help summary::before { content:"? "; display:inline-block; width:1.3em; height:1.3em; line-height:1.3em;
           text-align:center; border-radius:50%; background:var(--acc); color:#fff; margin-right:6px; font-size:.8rem; }
  details.help[open] summary { margin-bottom:6px; }
  details.help p { margin:6px 0; }
  details.help b { color:var(--ink); }
  .help-all { color:var(--mute); font-size:.85rem; margin:-6px 0 12px; }
  .chk { display:flex; align-items:center; gap:10px; padding:8px 0; color:var(--ink); font-size:1rem; }
  .chk input { width:22px; height:22px; accent-color:var(--acc); }
  .nets { margin:10px 0; }
  .net { display:flex; align-items:center; gap:8px; padding:8px 0; border-bottom:1px solid var(--bg); }
  .net span { flex:1; overflow:hidden; text-overflow:ellipsis; white-space:nowrap; }
  .net button { padding:6px 12px; font-size:.9rem; }
  .wiz { margin:10px 0; padding:12px; border-radius:10px; background:var(--bg); line-height:1.5; }
  .wiz .btns { margin-top:10px; }
  .gains { display:grid; grid-template-columns:repeat(4,1fr); gap:8px; }
  .gains input { width:100%; font-size:1rem; padding:10px 6px; margin:4px 0 8px; border-radius:8px;
                 border:1px solid var(--mute); background:var(--bg); color:var(--ink); }
  .grid2 { display:grid; grid-template-columns:1fr 1fr; gap:0 12px; }
</style>
</head>
<body>
<main>
  <div style="display:flex;justify-content:space-between;align-items:center;gap:8px">
    <h1 id="title">ESP32-S2 Motor Driver</h1>
    <span style="white-space:nowrap"><a href="/drive" style="color:var(--acc);font-weight:600;text-decoration:none">Drive page &rarr;</a>
    &nbsp;<a href="/logout" style="color:var(--mute);font-size:.85rem">Log out</a></span>
  </div>
  <div class="help-all">This page tests every device. To drive round a track use the <a href="/drive">Drive page</a>.<br>Tap <b style="color:var(--acc)">? What does this do?</b> on any card for an explanation.
    <a href="#" id="openAll">Open all</a></div>

  <div class="banner" id="apBanner" hidden>
    You're connected to the robot's own WiFi hotspot.<br>
    Want it on QUT, home or phone WiFi instead? Add networks in <a href="#wifi">WiFi setup</a> below.
  </div>

  <div class="card">
    <h2>Motors</h2>
    <div class="row"><label>Motor A</label><span class="val" id="aV">0 %</span></div>
    <input type="range" id="a" min="-100" max="100" value="0">
    <div class="row"><label>Motor B</label><span class="val" id="bV">0 %</span></div>
    <input type="range" id="b" min="-100" max="100" value="0">
    <div class="btns">
      <button id="link">Link A + B</button>
      <button id="center">Centre</button>
    </div>
    <details class="help"><summary>What does this do?</summary>
      <p>Each slider sets a motor's power from <b>-100 %</b> (full reverse) through <b>0</b> (stop) to <b>+100 %</b> (full forward).
      The percentage is the <b>PWM duty cycle</b>: the driver switches the battery on and off 20 000 times a second,
      and 50 % means "on half the time". It is power, not an exact speed - a loaded motor turns slower at the same %.</p>
      <p>Speed changes are smoothed by the <b>ramp time</b> (see Settings), so the motor speeds up gradually.</p>
      <p><b>Link A + B</b> makes both sliders move together - handy for driving straight.
      <b>Centre</b> sets both back to 0.</p>
      <p><b>Safety:</b> this page sends the slider values 5 times a second. If the robot stops hearing from it for 1 second
      (WiFi drops, phone locked, tab closed) it stops the motors by itself. This is called a <b>dead-man switch</b>.</p>
      <p>With <a href="#pid">Speed control</a> on, the sliders set <b>% of top speed</b> instead, and the encoders
      are used to hold that speed.</p>
      <p>Motor goes the wrong way? Tick <b>Reverse</b> in <a href="#setup">Robot setup</a>.</p>
    </details>
  </div>

  <div class="card btns">
    <button class="stop" id="stop">STOP</button>
    <button id="brake">Brake</button>
    <button id="demo">Demo</button>
    <button id="follow" style="grid-column:span 2">Line follow</button>
    <div style="grid-column:span 2">
    <details class="help"><summary>What does this do?</summary>
      <p><b>STOP</b> ends everything and turns the motor power off. The motors <b>coast</b> (freewheel) to a stop.</p>
      <p><b>Brake</b> connects each motor's two wires together inside the driver chip. A spinning motor acts as a generator,
      so shorting it makes it fight its own motion and stop much faster (<b>short brake</b>).</p>
      <p><b>Demo</b> plays a short pre-programmed dance: forward, reverse, spin left/right, full speed + brake.
      The servos wave along. Same as pressing <b>SW1</b> on the board.</p>
      <p><b>Line follow</b> uses the 8 line sensors to steer along a black line on a light floor.
      It steers harder the further the line is from the centre (a <b>proportional</b> controller) and stops if it
      loses the line for 0.4 s. Same as pressing <b>SW2</b>.</p>
      <p>Hold <b>any</b> button on the board for 1 second for an emergency stop.</p>
    </details>
    </div>
  </div>

  <div class="card">
    <h2>Encoders</h2>
    <div class="row"><label>A</label><span class="val" id="encA">-</span></div>
    <div class="row"><label>B</label><span class="val" id="encB">-</span></div>
    <button id="encReset" style="margin-top:8px;width:100%">Zero counts</button>
    <details class="help"><summary>What does this do?</summary>
      <p>Each motor has a small magnetic <b>encoder</b> on its shaft that sends two pulse signals as it turns.
      The first number is the <b>count</b> - how far the motor has turned since the last <b>Zero counts</b>.
      Forward makes it go up, reverse makes it go down.</p>
      <p>The second number is the speed in <b>cps = counts per second</b>. Every edge of both signals is counted,
      so one encoder pulse = 4 counts. If <code>ENC_COUNTS_PER_REV</code> is set in <code>config.h</code>
      the speed is shown in <b>rpm</b> (revolutions per minute) of the wheel instead.</p>
      <p>Example: 7-pulse encoder &times; 4 &times; 100:1 gearbox = 2800 counts per wheel turn.</p>
      <p>Count goes <b>down</b> while driving forward? Tick <b>Reverse encoder</b> in <a href="#setup">Robot setup</a>.</p>
    </details>
  </div>

  <div class="card">
    <h2>Line sensor</h2>
    <div class="line" id="line"></div>
    <details class="help"><summary>What does this do?</summary>
      <p>The 8 boxes are the 8 infra-red sensors on the line sensor bar (J15), sensor 1 on the left.
      Each sensor shines IR light at the floor and measures the reflection. A dark line reflects less,
      so that sensor's box fills in.</p>
      <p>Slide a strip of black electrical tape under the sensors to test it. If the boxes are filled everywhere
      <b>except</b> the line, your sensor works the other way round - change <code>LINE_ACTIVE_HIGH</code> in <code>config.h</code>.</p>
    </details>
  </div>

  <div class="card">
    <h2>Servos</h2>
    <div class="row"><label>Servo 1 (J8)</label><span class="val" id="s1V">off</span></div>
    <input type="range" id="s1" min="0" max="180" value="90">
    <div class="row"><label>Servo 2 (J9)</label><span class="val" id="s2V">off</span></div>
    <input type="range" id="s2" min="0" max="180" value="90">
    <details class="help"><summary>What does this do?</summary>
      <p>A hobby servo has a motor, gearbox and position sensor inside. The slider sets the angle it moves to and holds:
      <b>0&deg;</b> one end, <b>90&deg;</b> the middle, <b>180&deg;</b> the other end.</p>
      <p>The angle is sent as a pulse every 20 ms: 0.5 ms wide = 0&deg;, 1.5 ms = 90&deg;, 2.5 ms = 180&deg;.
      <b>off</b> means no pulses yet, so the servo is limp.</p>
      <p>A <b>continuous-rotation</b> servo spins instead: 90 = stop, lower = one way, higher = the other way.</p>
      <p>Plug in with the brown/black wire to GND. Servos run from the board's 5 V - big servos under load
      can draw more than it can supply. Servo mounted the other way round? Tick <b>Reverse servo</b> in <a href="#setup">Robot setup</a>.</p>
    </details>
  </div>

  <div class="card">
    <h2>LEDs</h2>
    <div class="btns4">
      <button class="led lr" data-c="r">R</button>
      <button class="led lg" data-c="g">G</button>
      <button class="led lb" data-c="b">B</button>
      <button class="led ly" data-c="y">Y</button>
    </div>
    <button id="ledAuto" style="margin-top:8px;width:100%">Auto (status) mode</button>
    <details class="help"><summary>What does this do?</summary>
      <p>Tap a colour to turn that LED on or off yourself. This switches off <b>Auto</b> mode.</p>
      <p>In <b>Auto (status) mode</b> the LEDs show what the robot is doing:</p>
      <p><b>Green</b> slow breathing = the program is running (if it freezes, the code has crashed)<br>
      <b>Blue</b> dim = a phone is connected to the robot's hotspot; blinking = this page is driving<br>
      <b>Yellow</b> = Demo or Line follow is running<br>
      <b>Red</b> = the motor driver is awake and a motor is moving</p>
    </details>
  </div>

  <div class="card" id="pid">
    <h2>Speed control (PID)</h2>
    <label class="chk"><input type="checkbox" id="pidOn"> Use the encoders to control speed and drive straight</label>
    <div class="status" id="pidInfo">-</div>
    <div class="gains" style="margin-top:8px">
      <div><label for="kp">Kp</label><input type="number" id="kp" step="0.1" min="0"></div>
      <div><label for="ki">Ki</label><input type="number" id="ki" step="0.5" min="0"></div>
      <div><label for="kd">Kd</label><input type="number" id="kd" step="0.01" min="0"></div>
      <div><label for="ks">Ks</label><input type="number" id="ks" step="0.5" min="0"></div>
    </div>
    <div class="btns">
      <button id="pidSave">Save gains</button>
      <button id="pidCal">Measure top speed</button>
    </div>
    <button id="pidLog" style="width:100%;margin-top:10px">Show live values in Terminal</button>
    <div class="msg" id="pidMsg"></div>
    <details class="help"><summary>What does this do?</summary>
      <p><b>Off</b>: a motor at 50 % gets half the battery's power. Two motors are never quite the same, so with equal
      power one wheel turns a little faster and the robot drifts in a curve.</p>
      <p><b>On</b>: 50 % means half of <b>top speed</b>. 50 times a second the robot measures each wheel's real speed
      from its encoder and adjusts the power until it matches. This is a <b>feedback loop</b> using a <b>PID controller</b>:</p>
      <p><b>P</b> (Kp) - push harder the further the wheel is from its target speed.<br>
      <b>I</b> (Ki) - add up the error over time, so any small error that stays gets removed (e.g. one gearbox is stiffer).<br>
      <b>D</b> (Kd) - react to how fast the speed is changing, to reduce overshoot. Often left at 0.<br>
      <b>Ks</b> - drive straight: while both wheels have the same target, it counts how far ahead one wheel has got
      and trims them back into step.</p>
      <p><b>Setting up:</b> first run <b>Check motors &amp; encoders</b> (Robot setup). Then, with the wheels off the ground,
      tap <b>Measure top speed</b>: both wheels run flat out for 2 s and the slower one sets 100 %. Then tick the box.</p>
      <p><b>Tuning:</b> tap <b>Show live values</b> and watch target (sp), measured (meas) and power (out) in the Terminal
      while you drive. Wobbly or oscillating? Lower Kp or Ki. Slow to reach speed? Raise Kp. Settles a bit low? Raise Ki.
      Still curves? Raise Ks.</p>
      <p><b>Safety:</b> if a wheel gets lots of power but its encoder says it isn't turning (unplugged, reversed or stalled),
      speed control switches itself off and stops the motors. The Terminal says why.</p>
    </details>
  </div>

  <div class="card">
    <h2>Settings</h2>
    <div class="row"><label>Ramp time (0 &rarr; 100 %)</label><span class="val" id="rV">-</span></div>
    <input type="range" id="ramp" min="0" max="2000" step="50">
    <details class="help"><summary>What does this do?</summary>
      <p><b>Ramp time</b> is how long a motor takes to go from stopped to full speed (and the same to slow down).
      The speed changes a little at a time instead of jumping.</p>
      <p>A gentle ramp stops wheelspin, stops the robot tipping over, is kinder to the gearbox, and avoids a big current
      surge from the battery. <b>0 ms</b> = instant (twitchy). Try 300-800 ms. Brake and STOP are always instant.</p>
      <p>This resets to the <code>config.h</code> value when the robot restarts.</p>
    </details>
  </div>

  <div class="card" id="setup">
    <h2>Robot setup <span style="text-transform:none;letter-spacing:0">(saved on the robot)</span></h2>
    <button id="wizStart" class="on" style="width:100%">Check motors &amp; encoders</button>
    <div class="wiz" id="wiz" hidden>
      <div id="wizText"></div>
      <div class="btns" id="wizBtns"></div>
    </div>
    <label style="display:block;margin-top:12px">Swap direction</label>
    <div class="grid2">
      <label class="chk"><input type="checkbox" data-rev="motorA"> Reverse motor A</label>
      <label class="chk"><input type="checkbox" data-rev="motorB"> Reverse motor B</label>
      <label class="chk"><input type="checkbox" data-rev="encA"> Reverse encoder A</label>
      <label class="chk"><input type="checkbox" data-rev="encB"> Reverse encoder B</label>
      <label class="chk"><input type="checkbox" data-rev="servo1"> Reverse servo 1</label>
      <label class="chk"><input type="checkbox" data-rev="servo2"> Reverse servo 2</label>
    </div>
    <label for="hostname" style="display:block;margin-top:10px">Robot name</label>
    <input type="text" id="hostname" maxlength="24" autocomplete="off" autocapitalize="none" spellcheck="false">
    <div class="btns">
      <button id="saveName" class="on">Save name</button>
      <button id="defName">Default name</button>
    </div>
    <div class="msg" id="setupMsg"></div>
    <label style="display:block;margin-top:14px">Robot password <span id="pwState"></span></label>
    <input type="password" id="pwCur" placeholder="Current password" autocomplete="current-password">
    <input type="password" id="pwNew" placeholder="New password (8+ characters)" autocomplete="new-password">
    <input type="password" id="pwNew2" placeholder="New password again" autocomplete="new-password">
    <div class="btns">
      <button id="pwSave" class="on">Change password</button>
      <button id="pwDef">Default password</button>
    </div>
    <div class="msg" id="pwMsg"></div>
    <details class="help"><summary>What does this do?</summary>
      <p><b>Check motors &amp; encoders</b> walks you through it: each wheel spins forward for 1 second, you say
      which way it turned, and the robot reverses the motor if needed. It then checks the encoder counts <b>up</b>
      going forward and reverses the encoder if not. Lift the wheels off the ground first. Do this once for each robot.</p>
      <p><b>Reverse motor</b>: if <b>+</b> on the slider makes a motor spin backwards (its two wires are swapped,
      or the motors face opposite ways on the chassis), tick this and the code swaps the direction for you - no rewiring.
      On a two-wheel robot you usually reverse <b>one</b> motor so that both sliders forward = robot forward.</p>
      <p><b>Reverse encoder</b>: tick this if the encoder count goes <b>down</b> when the motor drives forward.
      Fix the motor direction first, then the encoder.</p>
      <p><b>Reverse servo</b>: swaps 0&deg; and 180&deg;, for a servo mounted the other way round.</p>
      <p><b>Robot name</b> is used for the robot's own <b>hotspot name</b> and its web address
      <b>http://<i>name</i>.local</b>, so you can find your robot without knowing its IP address.
      Give every robot a different name, e.g. <code>team3</code> or <code>wall-e</code>.
      Use only lowercase letters, numbers and <b>-</b> (no spaces), up to 24 characters. The robot restarts to use the new name.</p>
      <p><b>.local</b> names work on iPhone, Mac, Windows and Linux. Some Android phones and some school/uni networks
      don't support them - use the IP address on the robot's screen instead.</p>
      <p><b>Robot password</b>: one password for logging in to these pages, the robot's own <b>hotspot</b>, and
      <b>firmware updates</b>. Every robot starts with <code>motors123</code> - set your team's own so other teams
      can't connect to and drive your robot. The robot restarts, then log in again with the new password
      (on the hotspot, reconnect with it too). Forgotten it? Hold <b>SW2</b> while switching on to go back to the default.</p>
      <p>These settings are kept in the robot's flash memory, even when new code is uploaded.</p>
    </details>
  </div>

  <div class="card" id="wifi">
    <h2>WiFi setup</h2>
    <div class="status" id="wifiNow">-</div>
    <label style="display:block;margin-top:12px">Saved networks - tried in this order at start-up</label>
    <div class="nets" id="netList"></div>
    <label>Add a network</label>
    <button id="scan" style="width:100%;margin:6px 0 10px">Scan for networks</button>
    <select id="nets" hidden></select>
    <label for="ssid">Network name (SSID)</label>
    <input type="text" id="ssid" autocomplete="off" autocapitalize="none" spellcheck="false">
    <label for="sec">Security</label>
    <select id="sec">
      <option value="personal">Personal - one WiFi password (home, phone hotspot)</option>
      <option value="enterprise">Enterprise - username + password (eduroam, work/uni)</option>
    </select>
    <div id="entFields" hidden>
      <label for="user">Username (e.g. name@university.edu)</label>
      <input type="text" id="user" autocomplete="off" autocapitalize="none" spellcheck="false">
      <label for="eap">Method</label>
      <select id="eap">
        <option value="peap">PEAP / MSCHAPv2 (most common)</option>
        <option value="ttls">TTLS</option>
      </select>
      <label for="ident">Anonymous identity (optional - leave blank to use username)</label>
      <input type="text" id="ident" autocomplete="off" autocapitalize="none" spellcheck="false">
    </div>
    <label for="pass">Password</label>
    <input type="password" id="pass" autocomplete="off">
    <label style="display:block;margin-bottom:10px"><input type="checkbox" id="showPass"> show password</label>
    <button id="save" class="on" style="width:100%">Save network</button>
    <div class="msg" id="wifiMsg"></div>
    <div class="btns" style="margin-top:10px">
      <button id="restart">Restart &amp; connect</button>
      <button id="forget">Forget all</button>
    </div>
    <details class="help"><summary>What does this do?</summary>
      <p><b>Hotspot mode</b>: with no network saved, the robot makes its own WiFi network (named after the robot,
      password = the <b>robot password</b>, <code>motors123</code> until you change it). Join it and this page opens at <b>http://192.168.4.1</b>.</p>
      <p><b>Saved networks</b>: the robot can remember up to 5 networks, e.g. QUT, home and your phone's hotspot.
      Each time it starts it scans, then tries the saved networks that are in range, from the top of the list down.
      If none of them work it makes its own hotspot, so you can always reach it. Tap <b>&uarr;</b> to try a network first,
      <b>&times;</b> to remove it.</p>
      <p>Saving a network while in hotspot mode restarts the robot straight away to connect. While it's on a network,
      new ones are just saved for next time - tap <b>Restart &amp; connect</b> to try now.
      Then browse to the IP address on the robot's screen (or <b>http://<i>robot-name</i>.local</b> where that works).</p>
      <p><b>Personal</b> = one password (home WiFi, phone hotspot). <b>Enterprise</b> = your own username + password
      (eduroam, uni/work WiFi).</p>
      <p>The ESP32-S2 only works with <b>2.4 GHz</b> WiFi, not 5 GHz. Hold <b>SW1</b> while powering on to forget the
      saved networks. Passwords are stored on the robot and never shown on this page.</p>
    </details>
  </div>

  <div class="card" id="ota">
    <h2>Firmware update (over WiFi)</h2>
    <label for="fwFile">Firmware file (.bin)</label>
    <input type="file" id="fwFile" accept=".bin" style="width:100%;margin:6px 0 10px">
    <button id="fwGo" class="on" style="width:100%">Upload &amp; install</button>
    <progress id="fwProg" max="100" value="0" style="width:100%;margin-top:10px" hidden></progress>
    <div class="msg" id="fwMsg"></div>
    <details class="help"><summary>What does this do?</summary>
      <p>Installs new code on the robot over WiFi - no USB cable needed. This is called an <b>OTA</b>
      (over-the-air) update. The robot stops its motors, writes the new code into a spare area of flash,
      checks it, and restarts. If anything goes wrong, it keeps running the old code.</p>
      <p><b>Getting the .bin file:</b> in PlatformIO press <b>Build</b> (&check;) and use
      <code>.pio/build/motordriver/firmware.bin</code>. In the Arduino IDE use
      <b>Sketch &rarr; Export Compiled Binary</b> and pick the <code>.bin</code> that does <b>not</b> say
      <i>bootloader</i>, <i>partitions</i> or <i>merged</i>.</p>
      <p>You can also upload straight from PlatformIO over WiFi (the <code>ota</code> environment) or pick the
      robot's network port in the Arduino IDE - see the README.</p>
      <p>Uploads from PlatformIO / the Arduino IDE ask for the <b>robot password</b>. Keep the robot powered and close by
      until it restarts. Your saved WiFi, name and reverse settings are kept.</p>
    </details>
  </div>

  <div class="card" id="terminal">
    <div class="row"><h2>Terminal</h2><button id="termClear" style="padding:4px 10px;font-size:.8rem">Clear</button></div>
    <pre id="termOut"></pre>
    <input id="termIn" placeholder="type a command, e.g. help" autocomplete="off" autocapitalize="none" spellcheck="false">
    <details class="help"><summary>What does this do?</summary>
      <p>Everything the robot prints to the USB serial monitor also appears here, so you can watch debug messages over WiFi.
      In your own code, print with <b>Log.printf(...)</b> / <b>Log.println(...)</b> instead of Serial and the text shows up
      both here and on USB.</p>
      <p>Type commands just like in the serial monitor: <b>help</b>, <b>info</b>, <b>wifi list</b>, <b>rev a</b>,
      <b>d 30 30</b>, <b>stop</b>... Up/down arrows recall earlier commands. Commands that wait for key presses
      (<b>enc</b>, <b>line</b>, <b>buttons</b>, <b>wifi set</b>, <b>selftest</b>, <b>blink</b>) only work over USB -
      this page already shows the live values.</p>
      <p>The robot keeps the last few kB of output, so you can open this page after something happened and still see it.</p>
    </details>
  </div>

  <div class="card">
    <h2>Status</h2>
    <div class="status" id="status">Connecting...</div>
    <details class="help"><summary>What does this do?</summary>
      <p><b>Mode</b>: what's in control - Idle, Web control, Demo, Line follow or Manual (serial commands).</p>
      <p><b>driver enabled / standby</b>: the motor driver's STBY pin. In standby the motors can't move
      (e.g. after an emergency stop - moving a slider wakes it again). <b>A / B %</b> = the actual power right now,
      which lags the sliders during the ramp.</p>
      <p><b>SW1 / SW2 / BOOT</b>: green dot = button held down. <b>OLED</b>: whether the screen was found on the I2C bus.</p>
      <p><b>Chip</b>: temperature sensor inside the ESP32 - rough, and runs warmer than the room.
      <b>heap</b>: free RAM in bytes. If it keeps going down over time, something is leaking memory.
      <b>up</b>: seconds since the robot started. <b>fw</b>: the firmware (program) version.</p>
    </details>
  </div>
</main>

<script>
// Logged out (robot restarted, password changed...)? Back to the log-in page.
const _fetch = window.fetch.bind(window);
window.fetch = (...a) => _fetch(...a).then(r => {
  if (r.status === 401) location.href = '/login?next=' + encodeURIComponent(location.pathname);
  return r;
});
const $ = id => document.getElementById(id);
const get = url => fetch(url).catch(() => {});
const esc = t => String(t).replace(/[&<>"]/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));
let linked = false, dirty = false, driving = false, robotName = 'motordriver', wifiMode = '';

// ---- motors
function show() { $('aV').textContent = $('a').value + ' %'; $('bV').textContent = $('b').value + ' %'; }
function moved(src) {
  if (linked) $(src === 'a' ? 'b' : 'a').value = $(src).value;
  show(); dirty = true; driving = true;
}
$('a').oninput = () => moved('a');
$('b').oninput = () => moved('b');
function zero() { $('a').value = 0; $('b').value = 0; show(); }
$('center').onclick = () => { zero(); dirty = true; };
$('link').onclick   = () => { linked = !linked; $('link').classList.toggle('on', linked); };
$('stop').onclick   = () => { zero(); driving = false; get('/api/stop'); };
$('brake').onclick  = () => { zero(); driving = false; get('/api/stop?brake=1'); };
$('demo').onclick   = () => { zero(); driving = false; get('/api/demo'); };
$('follow').onclick = () => { zero(); driving = false; get('/api/follow'); };
$('ramp').onchange  = () => get('/api/ramp?ms=' + $('ramp').value);
$('ramp').oninput   = () => $('rV').textContent = $('ramp').value + ' ms';
$('encReset').onclick = () => get('/api/enc/reset');

// Send slider values 5x a second while driving. The board stops the motors
// by itself if these messages stop arriving (dead-man safety).
setInterval(() => {
  const moving = $('a').value != 0 || $('b').value != 0;
  if (driving && (dirty || moving)) {
    dirty = false;
    get(`/api/drive?a=${$('a').value}&b=${$('b').value}`);
    if (!moving) driving = false;          // sliders centred: nothing more to send
  }
}, 200);

// Phone locked / tab switched: stop rather than wait for the dead-man.
document.addEventListener('visibilitychange', () => {
  if (document.hidden && driving) { zero(); driving = false; get('/api/stop'); }
});

// ---- servos
for (const n of [1, 2]) {
  $('s' + n).oninput = () => {
    $('s' + n + 'V').textContent = $('s' + n).value + '°';
    get(`/api/servo?n=${n}&deg=${$('s' + n).value}`);
  };
}

// ---- LEDs
const ledState = { r: 0, g: 0, b: 0, y: 0 };
document.querySelectorAll('.led').forEach(btn => btn.onclick = () => {
  const c = btn.dataset.c;
  ledState[c] = ledState[c] ? 0 : 255;
  btn.classList.toggle('on', !!ledState[c]);
  get(`/api/led?c=${c}&v=${ledState[c]}`);
});
$('ledAuto').onclick = () => {
  Object.keys(ledState).forEach(k => ledState[k] = 0);
  document.querySelectorAll('.led').forEach(b => b.classList.remove('on'));
  get('/api/led?c=auto');
};

// ---- WiFi setup
$('showPass').onchange = () => $('pass').type = $('showPass').checked ? 'text' : 'password';
function secChanged() { $('entFields').hidden = $('sec').value !== 'enterprise'; }
$('sec').onchange = secChanged;
let scanned = [];
$('nets').onchange = () => {
  $('ssid').value = $('nets').value;
  const n = scanned.find(x => x.ssid === $('nets').value);
  if (n) { $('sec').value = n.ent ? 'enterprise' : 'personal'; secChanged(); }
  ($('sec').value === 'enterprise' ? $('user') : $('pass')).focus();
};
function wifiMsg(text, isErr) { $('wifiMsg').textContent = text; $('wifiMsg').className = 'msg' + (isErr ? ' err' : ''); }

$('scan').onclick = async () => {
  $('scan').disabled = true; wifiMsg('Scanning...');
  for (let tries = 0; tries < 15; tries++) {
    try {
      const r = await (await fetch('/api/wifi/scan')).json();
      if (!r.scanning) {
        const nets = r.networks.sort((a, b) => b.rssi - a.rssi)
          .filter((n, i, all) => all.findIndex(m => m.ssid === n.ssid) === i);   // drop duplicates
        scanned = nets;
        $('nets').innerHTML = '<option value="">- pick a network -</option>' + nets.map(n =>
          `<option value="${esc(n.ssid)}">${esc(n.ssid)} (${n.rssi} dBm${n.open ? ', open' : ''}${n.ent ? ', enterprise' : ''})</option>`).join('');
        $('nets').hidden = false;
        wifiMsg(nets.length + ' networks found. Note: the ESP32-S2 only sees 2.4 GHz WiFi.');
        break;
      }
    } catch (e) {}
    await new Promise(res => setTimeout(res, 1000));
  }
  $('scan').disabled = false;
};

$('save').onclick = async () => {
  const ssid = $('ssid').value.trim(), pass = $('pass').value;
  const ent = $('sec').value === 'enterprise';
  if (!ssid) return wifiMsg('Enter or pick a network name first.', true);
  if (ent && (!$('user').value.trim() || !pass)) return wifiMsg('Enterprise WiFi needs a username and password.', true);
  if (!ent && pass && pass.length < 8) return wifiMsg('WiFi passwords are at least 8 characters.', true);
  const body = new URLSearchParams({ ssid, pass });
  if (ent) {
    body.append('type', 'enterprise');
    body.append('user', $('user').value.trim());
    body.append('ident', $('ident').value.trim());
    body.append('eap', $('eap').value);
  }
  try {
    const r = await fetch('/api/wifi/save', { method: 'POST', body });
    const t = await r.text();
    if (!r.ok) return wifiMsg(t, true);
    $('pass').value = '';
    if (wifiMode === 'AP') wifiMsg(t + ' Reconnect your phone to "' + ssid + '", then open the address shown on the robot\'s screen.');
    else { wifiMsg(t); loadNets(); }
  } catch (e) { wifiMsg('Could not reach the robot.', true); }
};

// Saved network list (passwords never come back from the robot)
function showNets(r) {
  $('netList').innerHTML = r.networks.length ? r.networks.map((n, i) =>
    `<div class="net"><span>${i + 1}. ${esc(n.ssid)}${n.ent ? ' (enterprise)' : ''}${i === r.joined ? ' <b class="ok">&check; connected</b>' : ''}</span>` +
    (i ? `<button data-top="${i}" title="Try first">&uarr;</button>` : '') +
    `<button data-del="${i}" title="Remove">&times;</button></div>`).join('')
    : '<div class="status">None yet - add one below. Until then the robot makes its own hotspot.</div>';
  $('netList').querySelectorAll('[data-top]').forEach(b => b.onclick = () =>
    fetch('/api/wifi/top?i=' + b.dataset.top).then(r => r.json()).then(showNets).catch(() => {}));
  $('netList').querySelectorAll('[data-del]').forEach(b => b.onclick = () => {
    if (confirm('Remove "' + r.networks[b.dataset.del].ssid + '"?'))
      fetch('/api/wifi/remove?i=' + b.dataset.del).then(r => r.json()).then(showNets).catch(() => {});
  });
}
function loadNets() { fetch('/api/wifi/list').then(r => r.json()).then(showNets).catch(() => {}); }
loadNets();

$('restart').onclick = async () => {
  if (!confirm('Restart the robot and look for the saved networks? This page will lose its connection.')) return;
  try { wifiMsg(await (await fetch('/api/wifi/restart')).text() + ' Check the robot\'s screen for its new address.'); }
  catch (e) { wifiMsg('Could not reach the robot.', true); }
};

$('forget').onclick = async () => {
  if (!confirm('Forget ALL saved networks and go back to hotspot mode?')) return;
  try { wifiMsg(await (await fetch('/api/wifi/forget')).text() + ' Join the robot\'s hotspot again.'); }
  catch (e) { wifiMsg('Could not reach the robot.', true); }
};

// ---- robot setup: reverse options + name
function setupMsg(text, cls) { $('setupMsg').textContent = text; $('setupMsg').className = 'msg ' + (cls || ''); }
function showConfig(c) {
  document.querySelectorAll('[data-rev]').forEach(cb => cb.checked = !!c[cb.dataset.rev]);
  if (document.activeElement !== $('hostname')) $('hostname').value = c.name;
}
fetch('/api/config').then(r => r.json()).then(showConfig).catch(() => {});
document.querySelectorAll('[data-rev]').forEach(cb => cb.onchange = async () => {
  try {
    const c = await (await fetch(`/api/config?${cb.dataset.rev}=${cb.checked ? 1 : 0}`)).json();
    showConfig(c);
    setupMsg('Saved.' + (cb.dataset.rev.startsWith('motor') ? ' Motors were stopped.' : ''), 'ok');
  } catch (e) { setupMsg('Could not reach the robot.', 'err'); }
});
async function saveName(name) {
  name = name.trim().toLowerCase();
  if (name && !/^[a-z0-9]([a-z0-9-]{0,22}[a-z0-9])?$/.test(name))
    return setupMsg('Use 1-24 characters: a-z, 0-9 and - (not at the start or end), no spaces.', 'err');
  if (!confirm('Rename the robot and restart it?')) return;
  try {
    const r = await fetch('/api/name', { method: 'POST', body: new URLSearchParams({ name }) });
    const t = await r.text();
    if (!r.ok) return setupMsg(t, 'err');
    setupMsg(t + ' Reconnect to the new hotspot name (hotspot mode), or browse to the new .local address.', 'ok');
  } catch (e) { setupMsg('Could not reach the robot.', 'err'); }
}
$('saveName').onclick = () => saveName($('hostname').value);
$('defName').onclick  = () => saveName('');
$('openAll').onclick = e => {
  e.preventDefault();
  const all = document.querySelectorAll('details.help');
  const open = ![...all].every(d => d.open);
  all.forEach(d => d.open = open);
  $('openAll').textContent = open ? 'Close all' : 'Open all';
};

// ---- guided motor + encoder direction check
function ask(html, choices) {
  $('wiz').hidden = false;
  $('wizText').innerHTML = html;
  $('wizBtns').innerHTML = '';
  return new Promise(res => choices.forEach(c => {
    const b = document.createElement('button');
    b.textContent = c; if (c === choices[0]) b.className = 'on';
    b.onclick = () => res(c);
    $('wizBtns').appendChild(b);
  }));
}
async function cfgSet(key, on) { return (await fetch(`/api/config?${key}=${on ? 1 : 0}`)).json(); }
async function checkMotor(m) {
  const L = m ? 'B' : 'A', motorKey = 'motor' + L, encKey = 'enc' + L;
  for (;;) {
    if (await ask(`<b>Motor ${L}</b> will spin for 1 second.<br>Make sure the wheels are off the ground.`,
                  ['Spin motor ' + L, 'Skip']) === 'Skip') return `Motor ${L}: skipped`;
    $('wizText').innerHTML = `Spinning motor ${L}...`; $('wizBtns').innerHTML = '';
    let r;
    try { r = await (await fetch('/api/setup/spin?m=' + m)).json(); } catch (e) { return `Motor ${L}: lost contact with the robot`; }
    const ans = await ask(`Which way did <b>wheel ${L}</b> turn?<br><span class="status">Forward = the way that would drive the robot forwards.</span>`,
                          ['Forward', 'Backward', "Didn't move"]);
    if (ans === 'Backward') {
      const c = await (await fetch('/api/config')).json();
      await cfgSet(motorKey, !c[motorKey]); showConfig(await (await fetch('/api/config')).json());
      await ask(`Motor ${L} is now <b>reversed</b> in software. Let's spin it again to check.`, ['OK']);
      continue;
    }
    if (ans !== 'Forward') {
      if (await ask(`Motor ${L} didn't move. Check the battery is connected and switched on, and the motor is plugged in.`,
                    ['Try again', 'Skip']) === 'Skip') return `Motor ${L}: did not move`;
      continue;
    }
    // Motor direction is right - now the encoder must count UP going forward
    if (r.counts > 20) return `Motor ${L}: OK, encoder counts up (+${r.counts}) &check;`;
    if (r.counts < -20) {
      const c = await (await fetch('/api/config')).json();
      await cfgSet(encKey, !c[encKey]); showConfig(await (await fetch('/api/config')).json());
      return `Motor ${L}: OK. Encoder was counting <b>backwards</b> (${r.counts}) - now reversed &check;`;
    }
    return `Motor ${L}: <span class="err">encoder hardly counted (${r.counts}) - check the encoder wires and its 3.3 V</span>`;
  }
}
$('wizStart').onclick = async () => {
  $('wizStart').disabled = true;
  const a = await checkMotor(0), b = await checkMotor(1);
  const res = `${a}<br>${b}`;
  const go = await ask(res + '<br><br>Measure top speed now for speed control (both wheels flat out for 2 s)?',
                       ['Measure top speed', 'Done']);
  if (go !== 'Done') {
    $('wizText').innerHTML = 'Measuring - wheels off the ground...'; $('wizBtns').innerHTML = '';
    const p = await (await fetch('/api/pid?cal=1')).json();
    showPid(p);
    await ask(res + '<br><br>' + (p.msg || '') + (p.max ? ` Top speed ${p.max} counts/s.` : ''), ['Done']);
  }
  $('wiz').hidden = true; $('wizStart').disabled = false;
};

// ---- speed control (PID)
function pidMsg(t, cls) { $('pidMsg').textContent = t; $('pidMsg').className = 'msg ' + (cls || ''); }
function showPid(p) {
  $('pidOn').checked = p.on;
  for (const k of ['kp', 'ki', 'kd', 'ks']) if (document.activeElement !== $(k)) $(k).value = p[k];
  $('pidInfo').textContent = p.max ? `Top speed: ${p.max} counts/s` : 'Top speed not measured yet - tap Measure top speed (wheels off the ground).';
  if (p.msg) pidMsg(p.msg, p.msg.includes('fail') || p.msg.includes('first') ? 'err' : 'ok');
}
fetch('/api/pid').then(r => r.json()).then(showPid).catch(() => {});
$('pidOn').onchange = async () => showPid(await (await fetch('/api/pid?on=' + ($('pidOn').checked ? 1 : 0))).json());
$('pidSave').onclick = async () => {
  const q = ['kp', 'ki', 'kd', 'ks'].map(k => `${k}=${encodeURIComponent($(k).value)}`).join('&');
  showPid(await (await fetch('/api/pid?' + q)).json()); pidMsg('Gains saved.', 'ok');
};
$('pidCal').onclick = async () => {
  if (!confirm('Wheels off the ground? Both motors will run at full speed for 2 seconds.')) return;
  pidMsg('Measuring...');
  try { showPid(await (await fetch('/api/pid?cal=1')).json()); } catch (e) { pidMsg('Could not reach the robot.', 'err'); }
};
$('pidLog').onclick = () => { fetch('/api/cmd', { method: 'POST', body: new URLSearchParams({ cmd: 'pid log' }) });
  location.hash = 'terminal'; };

// ---- OTA firmware update (XMLHttpRequest so we can show upload progress)
function fwMsg(text, cls) { $('fwMsg').textContent = text; $('fwMsg').className = 'msg ' + (cls || ''); }
$('fwGo').onclick = () => {
  const f = $('fwFile').files[0];
  if (!f) return fwMsg('Pick a .bin file first.', 'err');
  if (!f.name.endsWith('.bin')) return fwMsg('That is not a .bin firmware file.', 'err');
  if (!confirm(`Install ${f.name} (${Math.round(f.size / 1024)} kB)? The motors will stop.`)) return;
  zero(); driving = false;
  const fd = new FormData();
  fd.append('firmware', f, f.name);
  const x = new XMLHttpRequest();
  x.open('POST', '/api/update');
  x.upload.onprogress = e => {
    if (e.lengthComputable) { $('fwProg').value = e.loaded * 100 / e.total; fwMsg('Uploading... ' + Math.round(e.loaded * 100 / e.total) + ' %'); }
  };
  x.onload = () => {
    $('fwGo').disabled = false;
    if (x.status === 200) { $('fwProg').value = 100; fwMsg(x.responseText + ' This page will reconnect by itself.', 'ok'); }
    else { $('fwProg').hidden = true; fwMsg(x.responseText || 'Update failed.', 'err'); }
  };
  x.onerror = () => { $('fwGo').disabled = false; fwMsg('Lost connection during the upload - the old firmware is still running.', 'err'); };
  $('fwGo').disabled = true; $('fwProg').hidden = false; $('fwProg').value = 0;
  x.send(fd);
};

// ---- robot password
function pwMsg(t, cls) { $('pwMsg').textContent = t; $('pwMsg').className = 'msg ' + (cls || ''); }
async function setPassword(pw) {
  const body = new URLSearchParams({ current: $('pwCur').value, pw });
  try {
    const r = await fetch('/api/password', { method: 'POST', body });
    const t = await r.text();
    if (!r.ok) return pwMsg(t, 'err');
    pwMsg(t, 'ok');
    setTimeout(() => location.href = '/login', 6000);
  } catch (e) { pwMsg('Could not reach the robot.', 'err'); }
}
$('pwSave').onclick = () => {
  const a = $('pwNew').value, b = $('pwNew2').value;
  if (!$('pwCur').value) return pwMsg('Enter the current password first.', 'err');
  if (a.length < 8) return pwMsg('New password must be at least 8 characters (a WiFi rule).', 'err');
  if (/[\s"\\]/.test(a)) return pwMsg('No spaces, quotes or backslashes.', 'err');
  if (a !== b) return pwMsg('The two new passwords are different.', 'err');
  if (confirm('Change the robot password? It is also the hotspot and update password. The robot will restart.')) setPassword(a);
};
$('pwDef').onclick = () => {
  if (!$('pwCur').value) return pwMsg('Enter the current password first.', 'err');
  if (confirm('Go back to the default password (motors123)? Anyone could then drive this robot.')) setPassword('');
};

// ---- line sensor boxes
for (let i = 0; i < 8; i++) $('line').appendChild(document.createElement('div'));

// ---- status, twice a second
async function poll() {
  try {
    const s = await (await fetch('/api/status')).json();
    $('demo').classList.toggle('on', s.demo);
    $('follow').classList.toggle('on', s.follow);
    $('ledAuto').classList.toggle('on', s.ledsAuto);
    if (document.activeElement !== $('pidOn')) $('pidOn').checked = s.pid;
    if (document.activeElement !== $('ramp')) { $('ramp').value = s.ramp; $('rV').textContent = s.ramp + ' ms'; }
    if (!driving && !s.demo && !s.follow) zero();
    $('encA').textContent = `${s.encA}   ${s.rateA} ${s.rateUnit}`;
    $('encB').textContent = `${s.encB}   ${s.rateB} ${s.rateUnit}`;
    [...$('line').children].forEach((d, i) => d.classList.toggle('hit', (s.line >> i) & 1));
    $('apBanner').hidden = s.wifiMode !== 'AP';
    wifiMode = s.wifiMode;
    $('pwState').textContent = s.pwCustom ? '(set by you)' : '(still the default!)';
    $('pwState').className = s.pwCustom ? 'ok' : 'err';
    if (s.name !== robotName) { robotName = s.name; $('title').textContent = s.name; document.title = s.name; }
    $('wifiNow').innerHTML = s.wifiMode === 'AP'
      ? `Hotspot mode: <b>${esc(s.wifiSsid)}</b> at http://${s.wifiIp}` +
        (s.wifiSaved ? `<br>${s.wifiSaved} saved network(s) - none could be joined.` : '<br>No networks saved yet.') +
        (s.wifiError ? `<br><span class="err">${esc(s.wifiError)}</span>` : '')
      : `Joined <b>${esc(s.wifiSsid)}</b>${s.wifiSavedEnt ? ' (enterprise)' : ''} (${s.wifiRssi} dBm)<br>Address: http://${s.wifiIp} &nbsp; or &nbsp; http://${esc(s.name)}.local`;
    $('forget').hidden = !s.wifiSaved;
    $('restart').hidden = !s.wifiSaved;
    const dot = on => `<span class="dot ${on ? 'on' : ''}"></span>`;
    $('status').innerHTML =
      `Mode: <b>${s.mode}</b> &nbsp; driver ${s.stby ? 'enabled' : 'standby'} &nbsp; power A ${s.aNow}% B ${s.bNow}%` +
      (s.pid ? ` &nbsp; speed A ${s.measA}% B ${s.measB}%` : '') + '<br>' +
      `${dot(s.sw1)}SW1 &nbsp; ${dot(s.sw2)}SW2 &nbsp; ${dot(s.boot)}BOOT &nbsp; OLED ${s.oled ? 'ok' : 'missing'}<br>` +
      `Chip ${s.temp.toFixed(1)} &deg;C &nbsp; heap ${s.heap} B &nbsp; up ${s.uptime} s &nbsp; fw ${s.fw}`;
  } catch (e) { $('status').textContent = 'Lost connection...'; }
}
setInterval(poll, 500); poll(); show();
</script>
<script src="/term.js"></script>
</body>
</html>
)HTML";
