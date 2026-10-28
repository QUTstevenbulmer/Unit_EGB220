// =============================================================================
//  LoginPage.h  -  Asks for the robot password (see Settings.cpp / config.h).
//  %NAME%, %NEXT%, %ERROR% and %HINT% are filled in by WebUI.cpp.
// =============================================================================
#pragma once
#include <Arduino.h>

static const char LOGIN_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>%NAME% - log in</title>
<style>
  :root { --bg:#f4f5f7; --card:#fff; --ink:#1d2330; --mute:#6b7383; --acc:#2563eb; --err:#dc2626; }
  @media (prefers-color-scheme: dark) { :root { --bg:#12151b; --card:#1c212b; --ink:#e8ebf1; --mute:#98a1b3; --acc:#60a5fa; --err:#f87171; } }
  * { box-sizing:border-box; }
  body { margin:0; font-family:system-ui,sans-serif; background:var(--bg); color:var(--ink); }
  main { max-width:380px; margin:12vh auto 0; padding:16px; }
  .card { background:var(--card); border-radius:14px; padding:20px; box-shadow:0 1px 3px rgba(0,0,0,.08); }
  h1 { font-size:1.3rem; margin:0 0 4px; } p { color:var(--mute); margin:0 0 16px; line-height:1.45; font-size:.95rem; }
  input { width:100%; font-size:1.1rem; padding:12px; border-radius:10px; border:1px solid var(--mute); background:var(--bg); color:var(--ink); }
  button { width:100%; margin-top:12px; font-size:1.05rem; padding:13px; border:0; border-radius:10px; background:var(--acc); color:#fff; font-weight:600; }
  .err { color:var(--err); font-weight:600; margin:10px 0 0; min-height:1em; }
  .hint { margin-top:14px; font-size:.85rem; }
</style></head>
<body><main><div class="card">
  <h1>%NAME%</h1>
  <p>Enter this robot's password to drive it.</p>
  <form method="post" action="/login">
    <input type="hidden" name="next" value="%NEXT%">
    <input type="password" name="pw" placeholder="Robot password" autofocus autocomplete="current-password">
    <button type="submit">Log in</button>
  </form>
  <p class="err">%ERROR%</p>
  <p class="hint">%HINT%</p>
</div></main></body></html>
)HTML";
