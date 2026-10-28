// =============================================================================
//  DrivePage.h  -  The "Drive" page (http://<robot>/drive).
//
//  For driving round the track: the line-sensor view and position at the
//  top, a joystick underneath, big STOP / Line follow buttons, and the
//  Terminal. The full test page (every device) is at http://<robot>/
// =============================================================================
#pragma once
#include <Arduino.h>

static const char DRIVE_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Drive</title>
<style>
  :root { --bg:#f4f5f7; --card:#fff; --ink:#1d2330; --mute:#6b7383; --acc:#2563eb; --stop:#dc2626; --on:#16a34a; --line:#111827; }
  @media (prefers-color-scheme: dark) {
    :root { --bg:#12151b; --card:#1c212b; --ink:#e8ebf1; --mute:#98a1b3; --acc:#60a5fa; --stop:#f87171; --on:#4ade80; --line:#f3f4f6; }
  }
  * { box-sizing:border-box; }
  body { margin:0; font-family:system-ui,sans-serif; background:var(--bg); color:var(--ink); -webkit-user-select:none; user-select:none; }
  main { max-width:520px; margin:0 auto; padding:12px 16px 24px; }
  nav { display:flex; justify-content:space-between; align-items:center; margin-bottom:10px; }
  nav h1 { font-size:1.15rem; margin:0; }
  nav a { color:var(--acc); font-weight:600; text-decoration:none; font-size:.95rem; }
  h2 { font-size:.8rem; text-transform:uppercase; letter-spacing:.06em; color:var(--mute); margin:0 0 8px; }
  .card { background:var(--card); border-radius:14px; padding:14px; margin-bottom:12px; box-shadow:0 1px 3px rgba(0,0,0,.08); }
  .row { display:flex; justify-content:space-between; align-items:baseline; gap:8px; }
  .val { font-variant-numeric:tabular-nums; font-weight:600; }
  .mute { color:var(--mute); font-size:.9rem; }
  .sensors { display:grid; grid-template-columns:repeat(8,1fr); gap:6px; }
  .sensors div { height:36px; border-radius:8px; border:2px solid var(--mute); display:flex; align-items:flex-end;
                 justify-content:center; font-size:.75rem; color:var(--mute); padding-bottom:2px; }
  .sensors div.hit { background:var(--line); border-color:var(--line); color:var(--card); }
  .track { position:relative; height:16px; margin:12px 4px 4px; border-radius:8px; background:var(--bg); }
  .track::after { content:""; position:absolute; left:50%; top:-3px; bottom:-3px; width:2px; background:var(--mute); }
  .marker { position:absolute; top:-4px; width:24px; height:24px; margin-left:-12px; border-radius:50%;
            background:var(--acc); transition:left .15s; }
  .marker.lost { background:var(--mute); opacity:.4; }
  .btns { display:grid; grid-template-columns:1fr 1fr 1fr; gap:10px; }
  button { font-size:1rem; padding:14px 8px; border:0; border-radius:10px; cursor:pointer; background:var(--bg); color:var(--ink); font-weight:600; }
  button.stop { background:var(--stop); color:#fff; font-size:1.25rem; }
  button.on { background:var(--acc); color:#fff; }
  #pad { position:relative; width:min(280px, 72vw, 46vh); aspect-ratio:1; margin:6px auto 4px; border-radius:50%;
         background:radial-gradient(circle, var(--card) 0 30%, var(--bg) 31%); border:3px solid var(--mute); touch-action:none; }
  #pad::before, #pad::after { content:""; position:absolute; background:var(--mute); opacity:.35; }
  #pad::before { left:50%; top:8%; bottom:8%; width:2px; }
  #pad::after  { top:50%; left:8%; right:8%; height:2px; }
  #knob { position:absolute; left:50%; top:50%; width:30%; height:30%; margin:-15% 0 0 -15%; border-radius:50%;
          background:var(--acc); box-shadow:0 3px 10px rgba(0,0,0,.25); pointer-events:none; }
  input[type=range] { width:100%; height:36px; accent-color:var(--acc); }
  label { color:var(--mute); font-size:.9rem; }
  .chk { display:flex; align-items:center; gap:8px; margin-top:4px; }
  .grid3 { display:grid; grid-template-columns:repeat(3,1fr); gap:6px; text-align:center; }
  .grid3 .val { font-size:1.1rem; display:block; }
  #termOut { height:180px; overflow:auto; margin:0; padding:8px; border-radius:8px; background:#0b0f16; color:#d1fae5;
             font:12px/1.4 ui-monospace,Menlo,Consolas,monospace; white-space:pre-wrap; word-break:break-all; -webkit-user-select:text; user-select:text; }
  .term-in { display:flex; gap:8px; margin-top:8px; }
  .term-in input { flex:1; font:14px ui-monospace,Menlo,Consolas,monospace; padding:10px; border-radius:8px;
                   border:1px solid var(--mute); background:var(--bg); color:var(--ink); }
  .term-in button { padding:10px 14px; }
  details.help { margin-top:8px; font-size:.88rem; line-height:1.5; color:var(--mute); }
  details.help summary { cursor:pointer; color:var(--acc); font-weight:600; }
  details.help b { color:var(--ink); }
</style>
</head>
<body>
<main>
  <nav><h1 id="title">Drive</h1><span><a href="/">Test &amp; debug &rarr;</a> &nbsp;<a href="/logout" style="color:var(--mute);font-size:.85rem;font-weight:400">Log out</a></span></nav>

  <!-- 1. Line sensor + 2. joystick: together on one phone screen -->
  <div class="card">
    <div class="row"><h2>Line sensor</h2><span class="mute" id="posTxt">-</span></div>
    <div class="sensors" id="sensors"></div>
    <div class="track"><div class="marker lost" id="marker" style="left:50%"></div></div>
  </div>

  <div class="card">
    <div id="pad"><div id="knob"></div></div>
    <div class="row"><label for="max">Max speed</label><span class="val" id="maxV">60 %</span></div>
    <input type="range" id="max" min="10" max="100" step="5" value="60">
  </div>

  <!-- 3. Controls and motor status -->
  <div class="card">
    <div class="btns">
      <button class="stop" id="stop">STOP</button>
      <button id="brake">Brake</button>
      <button id="follow">Line follow</button>
    </div>
    <button id="pidBtn" style="width:100%;margin-top:10px">Speed control: off</button>
  </div>

  <div class="card">
    <div class="grid3">
      <div><span class="mute">Mode</span><span class="val" id="mode">-</span></div>
      <div><span class="mute">Motor A</span><span class="val" id="aNow">-</span></div>
      <div><span class="mute">Motor B</span><span class="val" id="bNow">-</span></div>
    </div>
    <div class="grid3" style="margin-top:8px">
      <div><span class="mute">Driver</span><span class="val" id="stby">-</span></div>
      <div><span class="mute">Enc A</span><span class="val" id="encA">-</span></div>
      <div><span class="mute">Enc B</span><span class="val" id="encB">-</span></div>
    </div>
    <label class="chk"><input type="checkbox" id="swap"><span>Motor A is the <b>right</b> wheel (tick if turning is backwards)</span></label>
    <details class="help"><summary>How do I drive?</summary>
      <p>Drag the blue knob: <b>up</b> = forward, <b>down</b> = reverse, <b>left/right</b> = turn. Push further to go faster.
      Let go and the robot stops. On a laptop use the <b>arrow keys</b> or <b>W A S D</b> (Space = stop).</p>
      <p><b>Max speed</b> limits how fast the joystick can go - start low on the track.</p>
      <p>The joystick mixes forward and turn into the two motors:
      left wheel = forward + turn, right wheel = forward - turn. Pushing straight sideways spins on the spot.</p>
      <p><b>Speed control</b> uses the wheel encoders to hold each wheel at the speed asked for, so the robot goes straight
      even if one motor is weaker. Set it up on the <a href="/#pid">Test &amp; debug</a> page first.</p>
      <p><b>Line follow</b> lets the robot drive itself along the line; touching the joystick takes back control.
      If the robot loses contact with this page for 1 s it stops by itself.</p>
      <p>Wheels going the wrong way? Fix that first with <b>Check motors &amp; encoders</b> in Robot setup on the
      <a href="/#setup">Test &amp; debug</a> page, then use the swap box above if left and right are mixed up.</p>
    </details>
  </div>

  <div class="card">
    <div class="row"><h2>Terminal</h2><button id="termClear" style="padding:4px 10px;font-size:.8rem">Clear</button></div>
    <pre id="termOut"></pre>
    <div class="term-in"><input id="termIn" placeholder="type a command, e.g. help" autocomplete="off" autocapitalize="none" spellcheck="false"></div>
    <details class="help"><summary>What is this?</summary>
      <p>Everything the robot prints to the USB serial monitor also appears here, so you can see debug messages over WiFi.
      In your own code use <b>Log.printf(...)</b> or <b>Log.println(...)</b> instead of Serial to send text here.</p>
      <p>You can type the same commands as in the serial monitor (try <b>help</b>, <b>info</b>, <b>d 30 30</b>, <b>stop</b>).
      A few that wait for key presses (live views, wifi set, selftest, blink) only work over USB.</p>
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
let sx = 0, sy = 0;              // joystick -1..1 (x right, y forward)
let padHeld = false, wasDriving = false;
const keys = {};

// ---- remembered settings (this browser only)
function load(k, d) { try { const v = localStorage.getItem(k); return v === null ? d : v; } catch (e) { return d; } }
function save(k, v) { try { localStorage.setItem(k, v); } catch (e) {} }
$('max').value = load('maxSpeed', 60);
$('swap').checked = load('swapAB', '0') === '1';
const showMax = () => $('maxV').textContent = $('max').value + ' %';
$('max').oninput = () => { showMax(); save('maxSpeed', $('max').value); };
$('swap').onchange = () => save('swapAB', $('swap').checked ? '1' : '0');
showMax();

// ---- sensor boxes
for (let i = 0; i < 8; i++) { const d = document.createElement('div'); d.textContent = i + 1; $('sensors').appendChild(d); }

// ---- joystick (touch / mouse)
const pad = $('pad'), knob = $('knob');
function setKnob(x, y) {
  const r = pad.clientWidth * 0.35;
  knob.style.transform = `translate(${x * r}px, ${-y * r}px)`;
}
function padMove(e) {
  const b = pad.getBoundingClientRect(), r = b.width * 0.35;
  let x = (e.clientX - (b.left + b.width / 2)) / r, y = -(e.clientY - (b.top + b.height / 2)) / r;
  const m = Math.hypot(x, y);
  if (m > 1) { x /= m; y /= m; }
  sx = x; sy = y; setKnob(x, y);
}
pad.addEventListener('pointerdown', e => { padHeld = true; pad.setPointerCapture(e.pointerId); padMove(e); });
pad.addEventListener('pointermove', e => { if (padHeld) padMove(e); });
const release = () => { padHeld = false; sx = 0; sy = 0; setKnob(0, 0); };
pad.addEventListener('pointerup', release);
pad.addEventListener('pointercancel', release);

// ---- keyboard: arrows / WASD (not while typing in the terminal)
const KEYMAP = { ArrowUp: 'f', w: 'f', ArrowDown: 'r', s: 'r', ArrowLeft: 'l', a: 'l', ArrowRight: 't', d: 't' };
document.addEventListener('keydown', e => {
  if (e.target.tagName === 'INPUT') return;
  const k = KEYMAP[e.key] || KEYMAP[e.key.toLowerCase()];
  if (k) { keys[k] = true; e.preventDefault(); }
  if (e.key === ' ') { e.preventDefault(); stopAll(); }
});
document.addEventListener('keyup', e => {
  const k = KEYMAP[e.key] || KEYMAP[e.key.toLowerCase()];
  if (k) keys[k] = false;
});

// ---- mix joystick into two motor speeds and send ~7x a second
function mix() {
  let x = sx, y = sy;
  if (!padHeld) {                                   // keyboard instead
    y = (keys.f ? 1 : 0) - (keys.r ? 1 : 0);
    x = ((keys.t ? 1 : 0) - (keys.l ? 1 : 0)) * (y ? 0.5 : 0.7);
  }
  let left = y + x * 0.7, right = y - x * 0.7;
  const m = Math.max(1, Math.abs(left), Math.abs(right));
  const max = +$('max').value;
  left = Math.round(left / m * max); right = Math.round(right / m * max);
  return $('swap').checked ? [right, left] : [left, right];
}
setInterval(() => {
  const [a, b] = mix();
  const active = padHeld || Object.values(keys).some(v => v);
  if (active) { get(`/api/drive?a=${a}&b=${b}`); wasDriving = true; }
  else if (wasDriving) { get('/api/drive?a=0&b=0'); wasDriving = false; }   // let go = stop
}, 150);

function stopAll(brake) { release(); Object.keys(keys).forEach(k => keys[k] = false); wasDriving = false;
  get('/api/stop' + (brake ? '?brake=1' : '')); }
$('stop').onclick   = () => stopAll(false);
$('brake').onclick  = () => stopAll(true);
let pidOn = false;
$('pidBtn').onclick = async () => {
  try {
    const p = await (await fetch('/api/pid?on=' + (pidOn ? 0 : 1))).json();
    if (p.msg) alert(p.msg + ' (Test & debug page > Speed control)');
  } catch (e) {}
  poll();
};
$('follow').onclick = () => { release(); wasDriving = false; get('/api/follow'); };
document.addEventListener('visibilitychange', () => { if (document.hidden && wasDriving) stopAll(false); });

// ---- status, 4x a second
async function poll() {
  try {
    const s = await (await fetch('/api/status')).json();
    let n = 0, sum = 0;
    [...$('sensors').children].forEach((d, i) => {
      const hit = (s.line >> i) & 1;
      d.classList.toggle('hit', !!hit);
      if (hit) { n++; sum += i - 3.5; }
    });
    if (n) {
      const pos = sum / n / 3.5;                    // -1 = far left, +1 = far right
      $('marker').style.left = (50 + pos * 50) + '%';
      $('marker').classList.remove('lost');
      $('posTxt').textContent = 'position ' + (pos >= 0 ? '+' : '') + pos.toFixed(2);
    } else {
      $('marker').classList.add('lost');
      $('posTxt').textContent = 'no line';
    }
    $('mode').textContent = s.mode;
    $('aNow').textContent = s.aNow + ' %';
    $('bNow').textContent = s.bNow + ' %';
    $('stby').textContent = s.stby ? 'on' : 'standby';
    $('follow').classList.toggle('on', s.follow);
    pidOn = s.pid;
    $('pidBtn').classList.toggle('on', s.pid);
    $('pidBtn').textContent = 'Speed control: ' + (s.pid ? 'ON (drive straight)' : 'off');
    $('encA').textContent = s.pid ? s.measA + ' %' : s.rateA + ' ' + s.rateUnit;
    $('encB').textContent = s.pid ? s.measB + ' %' : s.rateB + ' ' + s.rateUnit;
    if (s.name && $('title').textContent !== s.name) { $('title').textContent = s.name; document.title = s.name + ' - Drive'; }
  } catch (e) { $('mode').textContent = 'offline'; }
}
setInterval(poll, 250); poll();
</script>
<script src="/term.js"></script>
</body>
</html>
)HTML";
