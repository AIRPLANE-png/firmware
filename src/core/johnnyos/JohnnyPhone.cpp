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

// An authorization code is shown only on the StickS3 screen. The browser UI
// does not embed it. Control stays within the phone's current Wi-Fi network.
const char REMOTE_UI[] PROGMEM = R"JOHNNY(
<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>JohnnyOS Remote</title><style>
:root{color-scheme:dark}*{box-sizing:border-box}
body{font:16px system-ui,Arial;background:radial-gradient(circle at 75% 5%,#3046a1,#141b3b 52%,#080d1d);color:#f5f8ff;max-width:470px;margin:auto;min-height:100vh;padding:25px 22px;text-align:center}
h1{font-size:37px;letter-spacing:-2px;margin:16px 0 0}
i{font-style:normal;color:#9abfff}
p{color:#b4c4e5;line-height:1.5}
.card{background:#ffffff17;border:1px solid #ffffff29;border-radius:23px;padding:20px;margin:22px 0;box-shadow:0 10px 30px #0003}
input{width:100%;font:inherit;background:#101b39;border:1px solid #6679ab;border-radius:14px;padding:15px;color:white;letter-spacing:2px;text-align:center}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:13px}
button{min-height:74px;border:1px solid #b4d0ff7b;border-radius:21px;font-size:19px;font-weight:700;color:white;background:linear-gradient(140deg,#506cf5,#2943a2);touch-action:manipulation;box-shadow:0 5px 0 #111b3e;cursor:pointer}
button:active{transform:translateY(3px);box-shadow:none}
button.secondary{background:linear-gradient(140deg,#536584,#273852)}
#status{min-height:2em;font-size:13px;color:#a9d2ff;margin-top:15px}
small{color:#9caccf}
</style></head><body>
<h1>Johnny<i>OS</i></h1><p>Your StickS3, from your phone.</p>
<div class="card"><p>Type the 8-character code shown on the StickS3 screen.</p>
<input id="pin" maxlength="8" placeholder="DEVICE CODE" autocomplete="off" spellcheck="false">
</div><div class="grid">
<button class="secondary" onclick="go('prev')">◀ Previous</button>
<button onclick="go('next')">Next ▶</button>
<button onclick="go('select')">● Select</button>
<button class="secondary" onclick="go('back')">↶ Back</button>
</div><div id="status">Ready to connect</div>
<small>Local network only. Phone control is limited to menu navigation.</small>
<script>
async function go(k){
const token=document.getElementById('pin').value.trim().toUpperCase();
const status=document.getElementById('status');
if(!/^[0-9A-F]{8}$/.test(token)){status.textContent='Enter the 8-character code';return;}
try{
const res=await fetch('/cmd?k='+encodeURIComponent(k)+'&t='+encodeURIComponent(token),
{method:'POST',cache:'no-store'});
status.textContent=res.ok?'Command sent':res.status===403?'Code incorrect':'Remote unavailable';
}catch(e){status.textContent='Could not reach StickS3';}
}
</script></body></html>
)JOHNNY";

void waitForBack(const String &heading, const String &line1, const String &line2) {
    check(EscPress);
    while (true) {
        johnnyDrawWallpaper();
        tft.setTextColor(tft.color565(249, 252, 255));
        tft.drawString(heading, 12, 9, 2);
        tft.fillRoundRect(12, 38, 216, 68, 13, tft.color565(30, 43, 81));
        tft.drawCentreString(line1, 120, 52, 2);
        tft.setTextColor(tft.color565(172, 201, 243));
        tft.drawCentreString(line2, 120, 79, 1);
        tft.drawCentreString("Hold PREV to return", 120, 121, 1);
        if (check(EscPress)) break;
        delay(180);
    }
    check(EscPress);
}
void stopRemote() {
    if (!remoteServer) return;
    remoteServer->end();
    delete remoteServer;
    remoteServer = nullptr;
    queuedKey = 0;
}
bool isAuthorized(AsyncWebServerRequest *request) {
    return request->hasParam("t") &&
           request->getParam("t")->value().equalsIgnoreCase(accessCode);
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
    remoteServer->begin();
    return true;
}
} // namespace

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
