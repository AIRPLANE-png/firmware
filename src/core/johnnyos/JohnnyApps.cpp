#include "core/johnnyos/JohnnyApps.h"

#if defined(JOHNNYOS_STICKS3)
#include "core/display.h"
#include "core/johnnyos/JohnnyExtras.h"
#include "core/main_menu.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#include <Preferences.h>
#include <WiFi.h>
#include <globals.h>

namespace {
constexpr uint16_t NAVY = 0x080B;
constexpr uint16_t TEXT = 0xFFFF;
uint8_t cachedTheme = 255;

uint16_t color(uint8_t r, uint8_t g, uint8_t b) { return tft.color565(r, g, b); }
uint16_t panelColor() { return color(27, 36, 70); }
uint16_t dimColor() { return color(172, 196, 234); }
uint16_t tint() {
    switch (johnnyTheme()) {
        case 1: return color(187, 125, 255);
        case 2: return color(92, 238, 200);
        default: return color(112, 178, 255);
    }
}
void clearInputs() {
    check(NextPress); check(PrevPress); check(SelPress); check(EscPress);
}
void titleBar(const String &title, const String &subtitle) {
    johnnyDrawWallpaper();
    tft.setTextSize(1);
    tft.setTextColor(TEXT);
    tft.drawString(title, 10, 7, 1);
    tft.setTextColor(dimColor());
    tft.drawRightString(subtitle, tftWidth - 8, 8, 1);
    tft.drawFastHLine(10, 23, tftWidth - 20, color(58, 75, 130));
}
void footer(const String &message) {
    tft.fillRoundRect(6, tftHeight - 17, tftWidth - 12, 15, 6, color(19, 27, 54));
    tft.setTextSize(1);
    tft.setTextColor(dimColor());
    tft.drawCentreString(message, tftWidth / 2, tftHeight - 13, 1);
}
void note(const String &message) {
    titleBar("JohnnyOS", "INFO");
    tft.setTextColor(TEXT);
    tft.drawCentreString(message, tftWidth / 2, 59, 1);
    footer("Hold PREV to go back");
    clearInputs();
    while (!check(EscPress)) delay(25);
    clearInputs();
}
void showAI() {
    int selected = 0;
    const char *choices[] = {"Push-to-talk", "Play last voice", "AI service status", "Personality"};
    const char *personas[] = {"Friendly", "Tech", "Concise"};
    Preferences prefs;
    prefs.begin("johnnyos", true);
    uint8_t persona = prefs.getUChar("persona", 0) % 3;
    prefs.end();
    clearInputs();
    bool redrawAI = true;
    while (true) {
        if (check(EscPress)) break;
        if (check(PrevPress)) { selected = (selected + 3) % 4; redrawAI = true; }
        if (check(NextPress)) { selected = (selected + 1) % 4; redrawAI = true; }
        if (check(SelPress)) {
            if (selected == 0) johnnyPushToTalk();
            else if (selected == 1) johnnyPlayLastVoice();
            else if (selected == 2) note("Online AI needs a secure service");
            else {
                persona = (persona + 1) % 3;
                prefs.begin("johnnyos", false);
                prefs.putUChar("persona", persona);
                prefs.end();
            }
            redrawAI = true;
            clearInputs();
        }
        if (redrawAI) {
            redrawAI = false;
            titleBar("Johnny AI", WiFi.isConnected() ? "Wi-Fi ready" : "Offline");
            const int cx = 61, cy = 73;
            const int pulse = 2; // stable orb; no full-screen animation flashing
            tft.drawCircle(cx, cy, 28 + pulse, color(54, 81, 178));
            tft.fillCircle(cx, cy, 25 + pulse, color(54, 93, 207));
            tft.fillCircle(cx - 5, cy - 6, 17, color(132, 112, 247));
            tft.fillCircle(cx - 9, cy - 10, 8, color(210, 176, 255));
            tft.setTextColor(TEXT);
            tft.drawCentreString("AI", cx, cy - 3, 1);
            for (int i = 0; i < 4; ++i) {
                const int y = 27 + i * 22;
                const bool active = selected == i;
                if (active) tft.fillRoundRect(111, y, 121, 19, 7, color(66, 102, 212));
                tft.setTextColor(active ? TEXT : dimColor());
                String label = choices[i];
                if (i == 3) label = String("Style: ") + personas[persona];
                tft.drawString(label, 118, y + 5, 1);
            }
            tft.setTextColor(dimColor());
            tft.drawCentreString("NO AI SERVICE", cx, 111, 1);
            footer("Short: move  Hold: select / back");
        }
        delay(12);
    }
    clearInputs();
}
void showSystem() {
    // One stable screen per visit, rather than repainting the entire LCD at 5 Hz.
    titleBar("Device", "StickS3");
    tft.setTextColor(TEXT);
    tft.drawString("Battery", 15, 36, 1);
    tft.drawRightString(String(getBattery()) + "%", 217, 36, 1);
    tft.drawString("Flash", 15, 54, 1);
    tft.drawRightString(String(ESP.getFlashChipSize() / 1048576) + " MB", 217, 54, 1);
    tft.drawString("PSRAM", 15, 72, 1);
    tft.drawRightString(String(ESP.getPsramSize() / 1048576) + " MB", 217, 72, 1);
    tft.drawString("Free heap", 15, 90, 1);
    tft.drawRightString(String(ESP.getFreeHeap() / 1024) + " KB", 217, 90, 1);
    footer("Hold PREV to return");
    clearInputs();
    while (!check(EscPress)) delay(80);
    clearInputs();
}
void showSettings() {
    clearInputs();
    bool redrawSettings = true;
    while (true) {
        if (redrawSettings) {
        redrawSettings = false;
        titleBar("Appearance", "Saved in NVS");
        const char *themes[] = {"OCEAN", "VIOLET", "MINT"};
        tft.fillRoundRect(12, 34, 216, 62, 15, panelColor());
        tft.fillCircle(47, 65, 23, tint());
        tft.setTextColor(TEXT);
        tft.drawString("JohnnyOS theme", 89, 46, 1);
        tft.drawString(themes[johnnyTheme()], 89, 69, 2);
        footer("Next: change  Hold PREV: back");
        }
        if (check(EscPress)) break;
        if (check(NextPress) || check(SelPress)) { johnnyCycleTheme(); redrawSettings = true; }
        delay(30);
    }
    clearInputs();
}
void showWifi() {
    clearInputs();
    titleBar("Wi-Fi Scanner", "Scanning...");
    footer("Searching for real APs");
    if (WiFi.getMode() == WIFI_MODE_NULL) WiFi.mode(WIFI_STA);
    int found = WiFi.scanNetworks(false, true);
    if (found < 0) { note("Wi-Fi scanning failed"); return; }
    int selected = 0;
    const int count = min(found, 24);
    int lastSelected = -1;
    int lastStart = -1;
    bool redraw = true;
    auto row = [&](int idx, bool active, int start) {
        const int y = 27 + (idx - start) * 21;
        if (idx < start || idx >= start + 4 || idx >= count) return;
        tft.fillRoundRect(9, y, 222, 19, 7,
                          active ? color(59, 97, 188) : color(25, 38, 72));
        tft.setTextColor(active ? TEXT : dimColor());
        tft.drawString(WiFi.SSID(idx).substring(0, 23), 16, y + 5, 1);
        tft.drawRightString(String(WiFi.RSSI(idx)) + "dBm", 222, y + 5, 1);
    };
    while (true) {
        const int start = selected / 4 * 4;
        if (redraw || start != lastStart) {
            titleBar("Nearby networks", String(found) + " APs");
            if (count == 0) {
                tft.setTextColor(TEXT);
                tft.drawCentreString("No nearby networks", tftWidth / 2, 60, 1);
            } else {
                for (int i = start; i < min(start + 4, count); i++)
                    row(i, i == selected, start);
            }
            footer("Short: scroll  Hold PREV: back");
            redraw = false;
        } else if (lastSelected != selected) {
            row(lastSelected, false, start);
            row(selected, true, start);
        }
        lastSelected = selected;
        lastStart = start;
        if (check(EscPress)) break;
        if (count > 0 && check(NextPress)) selected = (selected + 1) % count;
        if (count > 0 && check(PrevPress)) selected = (selected + count - 1) % count;
        if (check(SelPress) && count > 0) {
            note(String("Ch ") + WiFi.channel(selected) + "  " + WiFi.SSID(selected).substring(0, 17));
            redraw = true;
        }
        delay(25);
    }
    WiFi.scanDelete();
    clearInputs();
}
void saveBest(int game, int score) {
    if (score <= 0) return;
    const char *keys[] = {"snake", "pong", "reaction", "racing", "flappy"};
    Preferences p;
    p.begin("johnnyos", false);
    const int best = p.getUInt(keys[game], 0);
    if (score > best) p.putUInt(keys[game], score);
    p.end();
}
// All five games below run entirely on the StickS3 with the two navigation keys.
// No animated browser / external assets are required.
void playGame(int game) {
    const char *names[] = {"SNAKE", "PONG", "REACTION", "MINI RACING", "FLAPPY"};
    int score = 0, lives = 3, tick = 0, lane = 1, obsLane = 0;
    int headX = 10, headY = 6, direction = 1, length = 3;
    int sx[72] = {}, sy[72] = {};
    sx[0] = 10; sy[0] = 6; sx[1] = 9; sy[1] = 6; sx[2] = 8; sy[2] = 6;
    int foodX = 15, foodY = 5;
    int paddleY = 65, cpuY = 65, bx = 110, by = 65, vx = 2, vy = 2;
    int obstacleY = 19, obstacleX = 215, gapY = 65;
    float birdY = 55, fall = 0;
    uint32_t started = millis(), lastFrame = 0, waitUntil = millis() + random(1800, 3600);
    bool reacted = false;
    clearInputs();
    titleBar(names[game], "0 pts");
    footer(game == 0 ? "Short: turn  Hold BACK: exit" :
           game == 1 ? "Short: paddle up/down" :
           game == 2 ? "Press either navigation button" :
           game == 3 ? "Short: switch lanes" : "Next: flap  Hold BACK: exit");
    int lastScore = -1;
    while (lives > 0) {
        bool left = check(PrevPress), right = check(NextPress), select = check(SelPress);
        if (check(EscPress)) break;
        if (game == 0) { // Snake: turn left or right, then automatically advance.
            if (left) direction = (direction + 3) % 4;
            if (right) direction = (direction + 1) % 4;
        } else if (game == 1) {
            if (left) paddleY -= 8;
            if (right) paddleY += 8;
            paddleY = constrain(paddleY, 37, 108);
        } else if (game == 2) {
            if ((left || right || select) && !reacted) {
                reacted = true;
                score = millis() >= waitUntil ? (int)(millis() - waitUntil) : -1;
                lives = 0;
            }
        } else if (game == 3) {
            if (left && lane > 0) lane--;
            if (right && lane < 2) lane++;
        } else if (game == 4) {
            if (right || select) fall = -4.7f;
        }
        int interval = game == 0 ? 160 : game == 2 ? 35 : 45;
        if (millis() - lastFrame < (uint32_t)interval) { delay(10); continue; }
        lastFrame = millis();
        tick++;
        if (game == 0) {
            int nx = headX + (direction == 1) - (direction == 3);
            int ny = headY + (direction == 2) - (direction == 0);
            if (nx < 0 || nx >= 23 || ny < 0 || ny >= 11) { lives = 0; }
            for (int i = 0; i < length - 1; ++i)
                if (nx == sx[i] && ny == sy[i]) lives = 0;
            if (!lives) break;
            for (int i = min(length, 70); i > 0; --i) { sx[i] = sx[i - 1]; sy[i] = sy[i - 1]; }
            headX = nx; headY = ny; sx[0] = nx; sy[0] = ny;
            if (nx == foodX && ny == foodY) {
                score += 10; length = min(length + 1, 70);
                foodX = random(1, 22); foodY = random(1, 10);
            }
        } else if (game == 1) {
            bx += vx; by += vy;
            if (by < 33 || by > 107) vy = -vy;
            cpuY += by > cpuY ? 2 : -2;
            cpuY = constrain(cpuY, 37, 108);
            if (bx > 210 && abs(by - cpuY) < 17) { vx = -abs(vx); score += 5; }
            if (bx < 26 && abs(by - paddleY) < 17) vx = abs(vx);
            if (bx < 13 || bx > 233) { lives--; bx = 120; by = 65; vx = -2; }
        } else if (game == 3) {
            obstacleY += 5;
            if (obstacleY > 98) {
                if (obsLane == lane) lives--;
                else score += 10;
                obstacleY = 27; obsLane = random(0, 3);
            }
        } else if (game == 4) {
            fall += .5f; birdY += fall;
            obstacleX -= 4;
            if (obstacleX < 13) { obstacleX = 231; gapY = random(48, 87); score += 10; }
            if (birdY < 29 || birdY > 110 ||
                (obstacleX > 40 && obstacleX < 57 && (birdY < gapY - 21 || birdY > gapY + 21)))
                lives = 0;
        }
        // Never repaint the wallpaper or title at the game frame rate.
        // Only the gameplay region is refreshed, and the score on changes.
        tft.fillRect(11, 27, 218, 92, color(12, 23, 49));
        if (score != lastScore) {
            tft.fillRoundRect(tftWidth - 65, 4, 57, 17, 6, color(22, 29, 54));
            tft.setTextColor(dimColor());
            tft.drawRightString(String(score) + " pts", tftWidth - 12, 8, 1);
            lastScore = score;
        }
        if (game == 0) {
            tft.fillRoundRect(13, 29, 185, 91, 5, color(18, 33, 57));
            tft.fillCircle(19 + foodX * 7, 34 + foodY * 8, 3, color(255, 101, 146));
            for (int i = 0; i < length; ++i)
                tft.fillRoundRect(16 + sx[i] * 7, 31 + sy[i] * 8, 7, 7, 2,
                    i ? color(71, 214, 160) : color(165, 255, 216));
            tft.setTextColor(dimColor());
            tft.drawString("TURN", 205, 47, 1);
            tft.drawString("L / R", 205, 64, 1);
        } else if (game == 1) {
            tft.fillRoundRect(13, 30, 215, 84, 8, color(17, 37, 65));
            tft.drawFastVLine(120, 31, 82, color(56, 79, 117));
            tft.fillRoundRect(20, paddleY - 11, 5, 23, 3, tint());
            tft.fillRoundRect(215, cpuY - 11, 5, 23, 3, color(255, 125, 178));
            tft.fillCircle(bx, by, 4, TEXT);
        } else if (game == 2) {
            uint16_t c = millis() >= waitUntil ? color(47, 204, 142) : color(213, 83, 113);
            tft.fillRoundRect(26, 36, 188, 70, 18, c);
            tft.setTextColor(TEXT);
            tft.drawCentreString(millis() >= waitUntil ? "PRESS NOW!" : "WAIT...", 120, 61, 2);
        } else if (game == 3) {
            for (int x = 42; x < 199; x += 51)
                tft.drawFastVLine(x, 27, 90, color(64, 86, 132));
            int carX = 68 + lane * 51;
            tft.fillRoundRect(carX - 11, 91, 23, 25, 7, tint());
            tft.fillRoundRect(68 + obsLane * 51 - 10, obstacleY, 21, 20, 6, color(255, 97, 129));
        } else if (game == 4) {
            tft.fillCircle(43, (int)birdY, 7, color(255, 226, 106));
            tft.fillCircle(46, (int)birdY - 2, 1, color(36, 38, 58));
            tft.fillRect(obstacleX, 26, 19, max(0, gapY - 47), color(67, 214, 154));
            tft.fillRect(obstacleX, gapY + 21, 19, max(0, 115 - gapY - 21), color(67, 214, 154));
        }
        delay(1);
    }
    if (game == 2) {
        if (score < 0) note("Too early! Try again");
        else note(String("Reaction: ") + score + " ms");
        // A faster reaction is better; it is not compared to the score-based high scores.
    } else {
        saveBest(game, score);
        titleBar("Game over", names[game]);
        tft.setTextColor(TEXT);
        tft.drawCentreString(String("Score: ") + score, 120, 64, 2);
        footer("Hold PREV to exit");
        while (!check(EscPress)) delay(25);
    }
    clearInputs();
}
void showGames() {
    options = {
        {"Snake", []() { playGame(0); }},
        {"Pong", []() { playGame(1); }},
        {"Reaction", []() { playGame(2); }},
        {"Mini Racing", []() { playGame(3); }},
        {"Flappy", []() { playGame(4); }}
    };
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "Johnny Games");
}
void showControls() {
    // This help screen is available at any time from the home-screen launcher.
    // It describes the ACTUAL Bruce StickS3 two-navigation-button mapping.
    const char *headings[] = {"HOME CONTROLS", "SELECT / BACK", "GAME CONTROLS", "AIR MOUSE", "PHONE REMOTE"};
    const char *lineA[] = {"KEY 2: Previous", "KEY 1: Hold / double", "Snake: Prev = left", "Tilt: move cursor", "Use Wi-Fi with phone"};
    const char *lineB[] = {"KEY 1: Next", "Select current app", "Next = right", "KEY 1: Left click", "Enter shown address"};
    const char *lineC[] = {"POWER: Reset / Off", "KEY 2: Hold / double", "Hold PREV to exit", "KEY 2: Right click", "Enter code on phone"};
    const char *lineD[] = {"", "Go back", "Pong: move paddle", "Hold PREV to exit", "Use Prev/Next/Select"};
    int page = 0;
    bool redraw = true;
    clearInputs();
    while (true) {
        if (check(EscPress)) break;
        if (check(NextPress)) { page = (page + 1) % 5; redraw = true; }
        if (check(PrevPress)) { page = (page + 4) % 5; redraw = true; }
        if (redraw) {
            redraw = false;
            titleBar("Controls", String(page + 1) + " / 5");
            tft.fillRoundRect(9, 31, 222, 83, 12, panelColor());
            tft.setTextColor(TEXT);
            tft.drawCentreString(headings[page], tftWidth / 2, 38, 2);
            tft.setTextColor(dimColor());
            tft.drawCentreString(lineA[page], tftWidth / 2, 63, 1);
            tft.drawCentreString(lineB[page], tftWidth / 2, 76, 1);
            tft.drawCentreString(lineC[page], tftWidth / 2, 89, 1);
            tft.drawCentreString(lineD[page], tftWidth / 2, 101, 1);
            footer("Prev / Next: pages  |  Hold Prev: exit");
        }
        delay(25);
    }
    clearInputs();
}

} // namespace

uint16_t johnnyRGB(uint8_t r, uint8_t g, uint8_t b) { return color(r, g, b); }
uint8_t johnnyTheme() {
    if (cachedTheme > 2) {
        Preferences prefs;
        prefs.begin("johnnyos", true);
        cachedTheme = prefs.getUChar("theme", 0) % 3;
        prefs.end();
    }
    return cachedTheme;
}
void johnnyCycleTheme() {
    cachedTheme = (johnnyTheme() + 1) % 3;
    Preferences prefs;
    prefs.begin("johnnyos", false);
    prefs.putUChar("theme", cachedTheme);
    prefs.end();
}
void johnnyDrawWallpaper() {
    const uint8_t theme = johnnyTheme();
    for (int y = 0; y < tftHeight; y += 3) {
        int d = y < tftHeight / 2 ? y : tftHeight - y;
        if (theme == 1) tft.fillRect(0, y, tftWidth, 3, color(23 + d / 4, 12 + d / 6, 56 + d / 2));
        else if (theme == 2) tft.fillRect(0, y, tftWidth, 3, color(8 + d / 7, 32 + d / 4, 37 + d / 3));
        else tft.fillRect(0, y, tftWidth, 3, color(10 + d / 5, 17 + d / 4, 47 + d / 2));
    }
    tft.fillCircle(tftWidth - 14, 8, 27, color(theme == 1 ? 54 : 32, theme == 2 ? 92 : 42, 119));
}
void johnnyDrawBoot() {
    const uint32_t start = millis();
    for (int frame = 0; frame < 12; frame++) {
        johnnyDrawWallpaper();
        tft.setTextColor(TEXT);
        tft.drawCentreString("JohnnyOS", tftWidth / 2, 39, 4);
        tft.setTextColor(dimColor());
        tft.drawCentreString("STICKS3 EDITION", tftWidth / 2, 76, 1);
        for (int j = 0; j < 3; ++j)
            tft.fillCircle(tftWidth / 2 - 13 + j * 13, 104, 2 + (j == frame % 3), tint());
        delay(85);
        if (check(AnyKeyPress)) break;
    }
    (void)start;
    tft.fillScreen(TFT_BLACK);
}
void JohnnyMenuItem::optionsMenu() {
    switch (_kind) {
        case AI: showAI(); break;
        case GAMES: showGames(); break;
        case WIFI: showWifi(); break;
        case SETTINGS: showSettings(); break;
        case SYSTEM: showSystem(); break;
        case PHONE: johnnyPhoneRemote(); break;
        case MOUSE: johnnyMouse(); break;
        case MEDIA: johnnyMediaRemote(); break;
        case HELP: showControls(); break;
    }
}
void JohnnyMenuItem::drawIcon(float) {
    // Launcher supplies its own icon art; this is a fallback for legacy menus.
    tft.fillCircle(iconCenterX, iconCenterY, 10, tint());
    tft.setTextColor(TEXT);
    tft.drawCentreString(getName().substring(0, 2), iconCenterX, iconCenterY - 4, 1);
}
#endif
