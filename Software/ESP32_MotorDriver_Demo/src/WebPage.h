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
</style>
</head>
<body>
<main>
  <h1>ESP32-S2 Motor Driver</h1>

  <div class="banner" id="apBanner" hidden>
    You're connected to the robot's own WiFi hotspot.<br>
    Want it on your home/school WiFi instead? Open <a href="#wifi">WiFi setup</a> below.
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
  </div>

  <div class="card btns">
    <button class="stop" id="stop">STOP</button>
    <button id="brake">Brake</button>
    <button id="demo">Demo</button>
    <button id="follow" style="grid-column:span 2">Line follow</button>
  </div>

  <div class="card">
    <h2>Encoders</h2>
    <div class="row"><label>A</label><span class="val" id="encA">-</span></div>
    <div class="row"><label>B</label><span class="val" id="encB">-</span></div>
    <button id="encReset" style="margin-top:8px;width:100%">Zero counts</button>
  </div>

  <div class="card">
    <h2>Line sensor</h2>
    <div class="line" id="line"></div>
  </div>

  <div class="card">
    <h2>Servos</h2>
    <div class="row"><label>Servo 1 (J8)</label><span class="val" id="s1V">off</span></div>
    <input type="range" id="s1" min="0" max="180" value="90">
    <div class="row"><label>Servo 2 (J9)</label><span class="val" id="s2V">off</span></div>
    <input type="range" id="s2" min="0" max="180" value="90">
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
  </div>

  <div class="card">
    <h2>Settings</h2>
    <div class="row"><label>Ramp time (0 &rarr; 100 %)</label><span class="val" id="rV">-</span></div>
    <input type="range" id="ramp" min="0" max="2000" step="50">
  </div>

  <div class="card" id="wifi">
    <h2>WiFi setup</h2>
    <div class="status" id="wifiNow">-</div>
    <button id="scan" style="width:100%;margin:10px 0">Scan for networks</button>
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
    <div class="btns">
      <button id="save" class="on">Save &amp; join</button>
      <button id="forget">Use hotspot</button>
    </div>
    <div class="msg" id="wifiMsg"></div>
  </div>

  <div class="card status" id="status">Connecting...</div>
</main>

<script>
const $ = id => document.getElementById(id);
const get = url => fetch(url).catch(() => {});
const esc = t => String(t).replace(/[&<>"]/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));
let linked = false, dirty = false, driving = false;

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
  if (driving && (dirty || $('a').value != 0 || $('b').value != 0)) {
    dirty = false;
    get(`/api/drive?a=${$('a').value}&b=${$('b').value}`);
  }
}, 200);

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
    wifiMsg(t + ' Reconnect your phone to "' + ssid + '", then open http://motordriver.local ' +
            'or the address shown on the robot\'s screen.');
  } catch (e) { wifiMsg('Could not reach the robot.', true); }
};

$('forget').onclick = async () => {
  if (!confirm('Forget the saved network and go back to hotspot mode?')) return;
  try { wifiMsg(await (await fetch('/api/wifi/forget')).text() + ' Join the robot\'s hotspot again.'); }
  catch (e) { wifiMsg('Could not reach the robot.', true); }
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
    if (document.activeElement !== $('ramp')) { $('ramp').value = s.ramp; $('rV').textContent = s.ramp + ' ms'; }
    if (!driving && !s.demo && !s.follow) zero();
    $('encA').textContent = `${s.encA}   ${s.rateA} ${s.rateUnit}`;
    $('encB').textContent = `${s.encB}   ${s.rateB} ${s.rateUnit}`;
    [...$('line').children].forEach((d, i) => d.classList.toggle('hit', (s.line >> i) & 1));
    $('apBanner').hidden = s.wifiMode !== 'AP';
    $('wifiNow').innerHTML = s.wifiMode === 'AP'
      ? `Hotspot mode: <b>${esc(s.wifiSsid)}</b> at ${s.wifiIp}` +
        (s.wifiSaved ? `<br>Saved network: ${esc(s.wifiSaved)}${s.wifiSavedEnt ? ' (enterprise)' : ''}` : '<br>No network saved yet.') +
        (s.wifiError ? `<br><span class="err">${esc(s.wifiError)}</span>` : '')
      : `Joined <b>${esc(s.wifiSsid)}</b>${s.wifiSavedEnt ? ' (enterprise)' : ''} (${s.wifiRssi} dBm)<br>Address: ${s.wifiIp} &nbsp; or http://motordriver.local`;
    $('forget').hidden = s.wifiMode === 'AP' && !s.wifiSaved;
    const dot = on => `<span class="dot ${on ? 'on' : ''}"></span>`;
    $('status').innerHTML =
      `Mode: <b>${s.mode}</b> &nbsp; driver ${s.stby ? 'enabled' : 'standby'} &nbsp; A ${s.aNow}% B ${s.bNow}%<br>` +
      `${dot(s.sw1)}SW1 &nbsp; ${dot(s.sw2)}SW2 &nbsp; ${dot(s.boot)}BOOT &nbsp; OLED ${s.oled ? 'ok' : 'missing'}<br>` +
      `Chip ${s.temp.toFixed(1)} &deg;C &nbsp; heap ${s.heap} B &nbsp; up ${s.uptime} s &nbsp; fw ${s.fw}`;
  } catch (e) { $('status').textContent = 'Lost connection...'; }
}
setInterval(poll, 500); poll(); show();
</script>
</body>
</html>
)HTML";
