#include "core/johnnyos/JohnnyExtras.h"

#if defined(JOHNNYOS_STICKS3)
#include "core/display.h"
#include "core/johnnyos/JohnnyApps.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <globals.h>

namespace {
AsyncWebServer *remoteServer = nullptr;
volatile uint8_t queuedKey = 0;
String accessCode;
uint32_t lastCommandMs = 0;
volatile bool settingsPending = false;
volatile uint8_t pendingNav = 0, pendingTheme = 0, pendingPower = 1;
volatile uint8_t pendingSound = 0, pendingPortrait = 0, pendingWallpaper = 0;
volatile int cachedBatteryLevel = -1;

// An authorization code is shown only on the StickS3 screen. The browser UI
// does not embed it. Control stays within the phone's current Wi-Fi network.
const char REMOTE_UI[] PROGMEM = R"JOHNNY(
<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="theme-color" content="#10152b">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-title" content="JohnnyOS">
<meta name="mobile-web-app-capable" content="yes">
<link rel="manifest" href="/manifest.webmanifest">
<link rel="icon" href="/icon.svg" type="image/svg+xml">
<title>JohnnyOS Control Center</title>
<style>
:root{color-scheme:dark;font-family:system-ui,-apple-system,"Segoe UI",sans-serif}
*{box-sizing:border-box}body{margin:0;color:#eef4ff;background:radial-gradient(ellipse at 50% 0,#263b73 0%,#111a37 48%,#090e20 100%);min-height:100vh}
main{width:min(100%,530px);padding:22px 19px 42px;margin:auto}
header{display:flex;align-items:center;justify-content:space-between;margin-bottom:23px}
.logo{font-size:29px;font-weight:850;letter-spacing:-1.5px}.logo span{color:#8ecaff}
.mini{font-size:12px;font-weight:700;border:1px solid #7597c566;background:#20345a88;color:#c3d8fb;padding:7px 11px;border-radius:100px}
h1{font-size:23px;letter-spacing:-.6px;margin:0 0 8px}h2{font-size:16px;margin:0 0 13px}
p{font-size:13px;line-height:1.5;color:#b5c6e7;margin:0 0 13px}
.card{border:1px solid #6578a64d;background:#f0f5ff0d;border-radius:22px;padding:19px;margin-bottom:15px;box-shadow:0 8px 30px #0002}
.row{display:flex;align-items:center;justify-content:space-between;gap:12px}
label{font-size:13px;font-weight:650;color:#d9e5ff}
input,select{display:block;width:100%;font:inherit;font-size:15px;background:#111d3b;color:#fff;border:1px solid #7997ca70;border-radius:12px;padding:12px;outline:none}
input:focus,select:focus{border-color:#9cc7ff;box-shadow:0 0 0 3px #5d9afd35}
#pin{text-transform:uppercase;letter-spacing:2px;max-width:170px;font-weight:750;text-align:center}
button{border:0;border-radius:15px;color:#fff;font:inherit;font-weight:750;cursor:pointer;touch-action:manipulation;transition:transform .09s,background .09s}
button:active{transform:scale(.97)}button:focus-visible{outline:3px solid #c9deff;outline-offset:3px}
.primary{background:linear-gradient(155deg,#619bff,#4258d7);padding:12px 16px;min-height:45px}
.ghost{background:#344970;padding:10px 13px;min-height:42px}
.nav{display:grid;grid-template-columns:1fr 1fr;gap:11px}
.nav button{min-height:74px;font-size:18px;background:linear-gradient(160deg,#4866c1,#273d83);border:1px solid #a1baff45}
.nav .important{background:linear-gradient(155deg,#6a8eff,#4565eb)}
.nav small{display:block;font-weight:500;font-size:11px;color:#d6e4ff;margin-top:3px}
.setting{margin-top:14px}.setting select{margin-top:7px}
hr{height:1px;border:0;background:#94b1e12b;margin:18px 0}
#status{font-size:13px;min-height:24px;color:#b2d5ff;padding:4px 2px 0}
.note{color:#9eafd2;font-size:12px;line-height:1.5}
.stats{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin:10px 0}
.stat{background:#142447;border:1px solid #7995c145;border-radius:12px;padding:12px 5px;text-align:center}
.stat strong{display:block;font-size:19px;color:#eff8ff}
.stat span{font-size:11px;color:#aabfe3}
.pill{font-size:11px;background:#244574;border-radius:100px;padding:6px 9px}
@media(hover:hover){button:hover{filter:brightness(1.12)}}
@media(prefers-reduced-motion:reduce){button{transition:none}}
</style></head><body><main>
<header><div class="logo">Johnny<span>OS</span></div><div class="mini">STICKS3 LOCAL</div></header>
<section class="card"><h1>Your control center</h1>
<p>Control your StickS3 and customize its look and button direction from this phone or computer.</p>
<div class="row"><label for="pin">Pairing code</label><input id="pin" maxlength="8" placeholder="8-CHAR CODE" autocomplete="off" spellcheck="false"></div>
<p style="margin-top:9px">Get the code from the StickS3 Phone Remote screen.</p>
<button class="primary" onclick="loadSettings()">Connect &amp; load settings</button>
<div id="status" role="status" aria-live="polite">Enter your device pairing code to begin.</div>
</section>
<section class="card"><div class="row"><h2>Live device dashboard</h2><span class="mini" id="live">OFFLINE</span></div>
<div class="stats"><div class="stat"><strong id="battery">--</strong><span>Battery</span></div>
<div class="stat"><strong id="heap">--</strong><span>Free memory</span></div>
<div class="stat"><strong id="wifi">--</strong><span>Wi-Fi</span></div></div>
<p class="note">Updates while this page is open. No cloud account needed.</p></section>
<section class="card"><div class="row"><h2>Remote control</h2><span class="mini">MENU</span></div>
<div class="nav">
<button onclick="go('prev')">◀ Previous<small>Move left / up</small></button>
<button onclick="go('next')">Next ▶<small>Move right / down</small></button>
<button class="important" onclick="go('select')">● Select<small>Open item</small></button>
<button onclick="go('back')">↶ Back<small>Exit a menu</small></button>
</div></section>
<section class="card"><div class="row"><h2>Customize JohnnyOS</h2><span class="mini">PREFERENCES</span></div>
<p>Saved on your StickS3. Hold / double-tap actions stay fixed so you can always Select and Back.</p>
<div class="setting"><label for="nav">Physical navigation taps</label>
<select id="nav"><option value="0">Standard: Key 1 Next / Key 2 Previous</option>
<option value="1">Reversed: Key 1 Previous / Key 2 Next</option></select></div>
<div class="setting"><label for="theme">Color theme</label><select id="theme">
<option value="0">Ocean blue</option><option value="1">Violet</option><option value="2">Mint</option></select></div>
<div class="setting"><label for="wallpaper">Wallpaper</label><select id="wallpaper"><option value="0">Soft gradient</option><option value="1">Midnight</option><option value="2">Aurora</option></select></div>
<div class="setting"><label for="orientation">Home orientation</label><select id="orientation"><option value="0">Landscape</option><option value="1">Portrait (home only)</option></select></div>
<div class="setting"><label for="sound">AI speaker volume</label><select id="sound"><option value="0">Off (default)</option><option value="1">On</option></select></div>
<div class="setting"><label for="power">Display power</label><select id="power">
<option value="0">Eco - 30% brightness</option><option value="1">Balanced - 58% brightness</option>
<option value="2">Bright - 85% brightness</option></select></div>
<hr><div class="row"><button class="primary" onclick="saveSettings()">Save settings</button>
<button class="ghost" onclick="loadSettings()">Refresh</button></div>
<p class="note" style="margin-top:15px">Settings are applied through the device menu. If you are inside an app, return to the home screen after saving.</p>
</section>
<section class="card"><div class="row"><h2>Game controller</h2><span class="mini">GAMEPAD</span></div>
<p>Works in supported JohnnyOS mini-games while the local remote is connected.</p>
<div class="nav"><button onclick="go('prev')">◀ Left</button><button onclick="go('next')">Right ▶</button><button class="important" onclick="go('select')">Action</button><button onclick="go('back')">Exit</button></div>
</section>
<section class="card"><div class="row"><h2>Offline assistant</h2><span class="mini">ON DEVICE</span></div>
<p>Ask about battery, controls, Wi-Fi, games, or settings. Answers come from a small built-in knowledge base, not a cloud model.</p>
<label for="question">Question</label>
<input id="question" maxlength="140" placeholder="How do I save battery?">
<div class="row" style="margin:12px 0"><button class="primary" onclick="askOffline()">Ask offline</button></div>
<p id="answer" aria-live="polite" style="white-space:pre-wrap;color:#e9f1ff">Your answer will appear here.</p>
</section>
<section class="card"><h2>Add to your phone</h2>
<p>Open your browser's Share/Menu and choose "Add to Home Screen". Your JohnnyOS control panel will open like an app while connected to your StickS3 Wi-Fi network.</p>
<p class="note">This local HTTP companion is a home-screen shortcut, not an App Store app. Some browsers require HTTPS for full PWA installation. The phone still needs local Wi-Fi to reach the StickS3.</p></section>
</main><script>
const $=id=>document.getElementById(id), say=t=>$('status').textContent=t;
let connected=false;
function renderState(d){
$('battery').textContent=d.battery>=0?d.battery+'%':'--';
$('heap').textContent=d.heap+' KB';$('wifi').textContent=d.wifi?'Ready':'Offline';
$('live').textContent='LIVE';
}
setInterval(async()=>{
if(!connected||document.visibilityState!=='visible'||!valid())return;
try{let r=await fetch('/state',{headers:headers(),cache:'no-store'});if(r.ok)renderState(await r.json());}
catch(e){$('live').textContent='RETRYING'}
},6000);

const code=()=>$('pin').value.trim().toUpperCase();
const valid=()=>{if(!/^[0-9A-F]{8}$/.test(code())){say('Enter the 8-character code shown on the device.');return false}return true};
function headers(){return {'X-Johnny-Code':code()}}
async function go(k){
if(!valid())return;
try{let r=await fetch('/cmd?k='+encodeURIComponent(k),{method:'POST',headers:headers(),cache:'no-store'});
say(r.ok?'Sent '+k:r.status===403?'Pairing code incorrect':r.status===429?'Device busy, tap again':'Remote command failed');}
catch(e){say('Device unreachable. Check Wi-Fi.')}
}
async function loadSettings(){
if(!valid())return;
try{let r=await fetch('/state',{headers:headers(),cache:'no-store'});
if(!r.ok)throw Error(r.status===403?'Pairing code incorrect':'Could not load settings');
let d=await r.json();$('nav').value=String(d.nav);$('theme').value=String(d.theme);$('power').value=String(d.power);
$('sound').value=String(d.sound);$('orientation').value=String(d.portrait);$('wallpaper').value=String(d.wallpaper);
renderState(d);connected=true;say('Connected! Settings loaded.');}
catch(e){say(e.message||'Connection failed')}
}
async function askOffline(){
if(!valid())return;
let q=$('question').value.trim();
if(!q){say('Type a question first.');return}
try{
let r=await fetch('/ask',{method:'POST',headers:{...headers(),'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({q}),cache:'no-store'});
if(!r.ok)throw Error(r.status===403?'Pairing code incorrect':'Offline assistant unavailable');
$('answer').textContent=await r.text();say('Answered locally on StickS3.');
}catch(e){say(e.message||'Cannot reach the device')}
}
async function saveSettings(){
if(!valid())return;
try{
let params=new URLSearchParams({nav:$('nav').value,theme:$('theme').value,power:$('power').value,sound:$('sound').value,portrait:$('orientation').value,wallpaper:$('wallpaper').value});
let r=await fetch('/settings?'+params.toString(),{method:'POST',headers:headers(),cache:'no-store'});
if(!r.ok)throw Error(r.status===403?'Pairing code incorrect':r.status===429?'Device busy, try again':'Could not apply preferences');
say('Settings sent together. Return to the device menu to apply.');
}catch(e){say(e.message||'Unable to save settings')}
}
</script></body></html>
)JOHNNY";

void waitForBack(const String &heading, const String &line1, const String &line2) {
    check(EscPress);
    // Pairing screen is static. Never repaint the entire display on a timer.
    johnnyDrawWallpaper();
        tft.setTextColor(tft.color565(249, 252, 255));
        tft.drawString(heading, 12, 9, 2);
        tft.fillRoundRect(12, 38, 216, 68, 13, tft.color565(30, 43, 81));
        tft.drawCentreString(line1, 120, 52, 2);
        tft.setTextColor(tft.color565(172, 201, 243));
        tft.drawCentreString(line2, 120, 79, 1);
        tft.drawCentreString("Hold PREV to return", 120, 121, 1);
    while (!check(EscPress)) delay(80);
    check(EscPress);
}
void stopRemote() {
    if (!remoteServer) return;
    remoteServer->end();
    delete remoteServer;
    remoteServer = nullptr;
    queuedKey = 0;
    settingsPending = false;
}
bool isAuthorized(AsyncWebServerRequest *request) {
    // Never put the pairing secret into the URL. Browsers send it in a
    // custom header; cross-origin forms cannot silently supply this header.
    return request->hasHeader("X-Johnny-Code") &&
           request->getHeader("X-Johnny-Code")->value().equalsIgnoreCase(accessCode);
}
bool startRemote() {
    if (remoteServer) return true;
    if (!WiFi.isConnected() && !WiFi.AP.started()) {
        if (!wifiConnectMenu(WIFI_STA)) return false;
    }
    if (!WiFi.isConnected() && !WiFi.AP.started()) return false;

    accessCode = String(esp_random(), HEX);
    accessCode.toUpperCase();
    while (accessCode.length() < 8) accessCode = "0" + accessCode;
    remoteServer = new AsyncWebServer(8181);
    remoteServer->on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
        AsyncWebServerResponse *r = req->beginResponse_P(200, "text/html; charset=utf-8", REMOTE_UI);
        r->addHeader("Cache-Control", "no-store");
        r->addHeader("X-Frame-Options", "DENY");
        r->addHeader("Content-Security-Policy", "default-src 'self' 'unsafe-inline'; frame-ancestors 'none'");
        req->send(r);
    });
    remoteServer->on("/manifest.webmanifest", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send(200,"application/manifest+json",
            R"JS({"name":"JohnnyOS Control Center","short_name":"JohnnyOS","start_url":"/","scope":"/","display":"standalone","background_color":"#10152b","theme_color":"#10152b","icons":[{"src":"/icon.svg","sizes":"any","type":"image/svg+xml","purpose":"any maskable"}]})JS");
    });
    remoteServer->on("/icon.svg", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send(200,"image/svg+xml",
            R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 192 192"><rect width="192" height="192" rx="44" fill="#152755"/><circle cx="96" cy="96" r="61" fill="#547af6"/><path d="M55 99l24 23 60-59" fill="none" stroke="#e8f4ff" stroke-width="15" stroke-linecap="round" stroke-linejoin="round"/></svg>)SVG");
    });
    remoteServer->on("/cmd", HTTP_POST, [](AsyncWebServerRequest *req) {
        if (!isAuthorized(req)) { req->send(403, "text/plain", "Unauthorized"); return; }
        if (!req->hasParam("k")) { req->send(400, "text/plain", "Missing command"); return; }
        // Single-command mailbox; avoid fast repeats. Never execute device
        // features from an AsyncWebServer callback or expose arbitrary tools.
        if (millis() - lastCommandMs < 180 || queuedKey != 0) {
            req->send(429, "text/plain", "Busy"); return;
        }
        String k = req->getParam("k")->value();
        if (k == "prev") queuedKey = 1;
        else if (k == "next") queuedKey = 2;
        else if (k == "select") queuedKey = 3;
        else if (k == "back") queuedKey = 4;
        else { req->send(400, "text/plain", "Invalid command"); return; }
        lastCommandMs = millis();
        req->send(200, "text/plain", "OK");
    });
    remoteServer->on("/state", HTTP_GET, [](AsyncWebServerRequest *req) {
        if (!isAuthorized(req)) { req->send(403, "text/plain", "Unauthorized"); return; }
        String body = String("{\"nav\":") + (johnnyNavigationSwap() ? "1" : "0") +
                      ",\"theme\":" + String(johnnyTheme()) +
                      ",\"power\":" + String(johnnyPowerMode()) +
                      ",\"sound\":" + String(johnnySoundEnabled() ? 1 : 0) +
                      ",\"portrait\":" + String(johnnyPortrait() ? 1 : 0) +
                      ",\"wallpaper\":" + String(johnnyWallpaperStyle()) +
                      ",\"battery\":" + String(cachedBatteryLevel) +
                      ",\"heap\":" + String(ESP.getFreeHeap()/1024) +
                      ",\"wifi\":" + String(WiFi.isConnected() ? "true" : "false") + "}";
        AsyncWebServerResponse *res = req->beginResponse(200, "application/json", body);
        res->addHeader("Cache-Control", "no-store");
        req->send(res);
    });
    remoteServer->on("/settings", HTTP_POST, [](AsyncWebServerRequest *req) {
        if (!isAuthorized(req)) { req->send(403, "text/plain", "Unauthorized"); return; }
        if (!req->hasParam("nav") || !req->hasParam("theme") ||
            !req->hasParam("power") || !req->hasParam("sound") ||
            !req->hasParam("portrait") || !req->hasParam("wallpaper")) {
            req->send(400, "text/plain", "Missing settings"); return;
        }
        String nav = req->getParam("nav")->value();
        String theme = req->getParam("theme")->value();
        String power = req->getParam("power")->value();
        String sound = req->getParam("sound")->value();
        String portrait = req->getParam("portrait")->value();
        String wallpaper = req->getParam("wallpaper")->value();
        if ((nav != "0" && nav != "1") ||
            (sound != "0" && sound != "1") ||
            (portrait != "0" && portrait != "1") ||
            (theme != "0" && theme != "1" && theme != "2") ||
            (wallpaper != "0" && wallpaper != "1" && wallpaper != "2") ||
            (power != "0" && power != "1" && power != "2")) {
            req->send(400, "text/plain", "Invalid settings"); return;
        }
        if (settingsPending) { req->send(429, "text/plain", "Busy"); return; }
        pendingNav = (uint8_t)nav.toInt();
        pendingTheme = (uint8_t)theme.toInt();
        pendingPower = (uint8_t)power.toInt();
        pendingSound = (uint8_t)sound.toInt();
        pendingPortrait = (uint8_t)portrait.toInt();
        pendingWallpaper = (uint8_t)wallpaper.toInt();
        settingsPending = true;
        req->send(202, "text/plain", "Queued");
    });
    remoteServer->on("/ask", HTTP_POST, [](AsyncWebServerRequest *req) {
        if (!isAuthorized(req)) { req->send(403, "text/plain", "Unauthorized"); return; }
        if (!req->hasParam("q", true)) { req->send(400, "text/plain", "Missing question"); return; }
        String q = req->getParam("q", true)->value();
        if (q.length() > 140) { req->send(413, "text/plain", "Question too long"); return; }
        // Lightweight, bounded intent responses. No AI key, model, cloud or
        // unrestricted device control is exposed to the HTTP worker.
        String answer = johnnyOfflineReply(q);
        AsyncWebServerResponse *res = req->beginResponse(200, "text/plain; charset=utf-8", answer);
        res->addHeader("Cache-Control", "no-store");
        req->send(res);
    });
    remoteServer->begin();
    return true;
}
} // namespace

void johnnyRemoteUpdateBattery(int battery) { cachedBatteryLevel = battery; }

bool johnnyRemotePopSettings(uint8_t &nav, uint8_t &theme, uint8_t &power,
                             uint8_t &sound, uint8_t &portrait, uint8_t &wallpaper) {
    if (!settingsPending) return false;
    nav = pendingNav;
    theme = pendingTheme;
    power = pendingPower;
    sound = pendingSound;
    portrait = pendingPortrait;
    wallpaper = pendingWallpaper;
    settingsPending = false;
    return true;
}

bool johnnyRemotePop(uint8_t &key) {
    if (queuedKey == 0) return false;
    key = queuedKey;
    queuedKey = 0;
    return true;
}

void johnnyPhoneRemote() {
    if (remoteServer) {
        options = {
            {"Show pairing code", []() {
                String ip = WiFi.isConnected() ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
                waitForBack(String("Phone Remote"), accessCode, ip + ":8181");
            }},
            {"Stop phone remote", []() { stopRemote(); }},
        };
        addOptionToMainMenu();
        loopOptions(options, MENU_TYPE_SUBMENU, "Phone Remote");
        return;
    }
    if (!startRemote()) {
        waitForBack("Phone Remote", "Wi-Fi required", "Connect through Wi-Fi menu");
        return;
    }
    String ip = WiFi.isConnected() ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    waitForBack("Phone Remote", accessCode, ip + ":8181");
}
#endif
