// =============================================================================
//  TermJs.h  -  The web Terminal, shared by both pages (served as /term.js).
//
//  Shows everything the firmware prints with Log.print / Log.printf (the same
//  text as the USB serial monitor), and sends typed commands to the robot.
//  The page needs: <pre id="termOut">, <input id="termIn">, <button id="termClear">
// =============================================================================
#pragma once
#include <Arduino.h>

static const char TERM_JS[] PROGMEM = R"JS(
(() => {
  const out = document.getElementById('termOut'), inp = document.getElementById('termIn');
  if (!out) return;
  let next = -1, busy = false, follow = true;
  const hist = []; let hi = 0;

  // Stop auto-scrolling while you scroll up to read; resume at the bottom.
  out.addEventListener('scroll', () => { follow = out.scrollTop + out.clientHeight >= out.scrollHeight - 24; });

  function add(text) {
    if (!text) return;
    out.textContent += text.replace(/\r/g, '');
    if (out.textContent.length > 30000) out.textContent = out.textContent.slice(-20000);
    if (follow) out.scrollTop = out.scrollHeight;
  }

  // Ask for new output twice a second. X-Next = where to carry on from.
  async function pull() {
    if (busy) return;
    busy = true;
    try {
      const r = await fetch('/api/log?from=' + next);
      const n = r.headers.get('X-Next');
      add(await r.text());
      if (n !== null) next = n;
    } catch (e) {}
    busy = false;
  }
  setInterval(pull, 500); pull();

  async function send(cmd) {
    try { await fetch('/api/cmd', { method: 'POST', body: new URLSearchParams({ cmd }) }); }
    catch (e) { add('\n(could not reach the robot)\n'); }
    pull();
  }

  if (inp) inp.addEventListener('keydown', e => {
    if (e.key === 'Enter' && inp.value.trim()) {
      hist.push(inp.value); hi = hist.length;
      send(inp.value); inp.value = ''; follow = true;
    } else if (e.key === 'ArrowUp' && hi > 0) { inp.value = hist[--hi]; e.preventDefault(); }
    else if (e.key === 'ArrowDown') { hi = Math.min(hi + 1, hist.length); inp.value = hist[hi] || ''; e.preventDefault(); }
  });
  const clr = document.getElementById('termClear');
  if (clr) clr.onclick = () => { out.textContent = ''; };
})();
)JS";
