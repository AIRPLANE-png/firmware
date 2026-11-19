#include "core/johnnyos/JohnnyExtras.h"

#if defined(JOHNNYOS_STICKS3)
#include "core/johnnyos/JohnnyApps.h"
#include "core/display.h"
#include "core/utils.h"
#include <WiFi.h>
#include <globals.h>

// JohnnyOS Offline Assistant is a tiny deterministic intent engine, not a
// generative language model. It runs entirely on ESP32-S3; no network needed.
String johnnyOfflineReply(const String &question) {
    String q = question;
    q.trim();
    q.toLowerCase();
    if (q.length() > 140) q = q.substring(0, 140);
    auto has = [&](const char *word) { return q.indexOf(word) >= 0; };

    if (q.length() == 0 || has("what can") || has("help") || has("what do you do")) {
        return "I work fully offline! Ask about the battery, Wi-Fi, Bluetooth, buttons, games, files, power modes, or JohnnyOS settings. I use built-in answers, not a cloud model.";
    }
    if (has("battery") || has("charge") || has("power left")) {
        return String("Your battery reads ") + String(getBattery()) +
               "%. Choose Eco under Themes to dim earlier and save battery. Radios and a bright screen use more power.";
    }
    if (has("wifi") || has("wi-fi") || has("internet") || has("network")) {
        if (WiFi.isConnected())
            return String("Wi-Fi is connected to ") + WiFi.SSID() +
                   ". Johnny AI's offline answers do not need internet.";
        return "Wi-Fi is disconnected. That's OK for offline answers. Open Wi-Fi Scan to find nearby networks, or Bruce Wi-Fi tools to connect.";
    }
    if (has("bluetooth") || has("ble") || has("mouse")) {
        return "Air Mouse uses Bluetooth LE and the StickS3 motion sensor. Pair 'JohnnyOS Mouse' on a computer. Tilt to move; the device shows click controls.";
    }
    if (has("button") || has("control") || has("key ") ||
        has("next") || has("previous") || has("back")) {
        return johnnyNavigationSwap()
            ? "Your taps are reversed: Key 1 = Previous, Key 2 = Next. Hold Key 1 to Select and Key 2 to Back. Two quick taps move two apps. Change taps in Controls or the browser."
            : "Key 1 tap = Next; Key 2 tap = Previous. Hold or double Key 1 to Select; Key 2 to Back. Change taps in Controls or the browser.";
    }
    if (has("file") || has("folder") || has("storage") || has("sd card")) {
        return "Open Files for Bruce's local file manager. Be careful deleting files. The JohnnyOS phone remote currently changes menu settings, not file contents.";
    }
    if (has("game") || has("snake") || has("pong") || has("racing") || has("flappy")) {
        return "The Games app includes Snake, Pong, Reaction, Mini Racing and Flappy. Short taps move or steer. Hold Back to exit a game.";
    }
    if (has("theme") || has("color") || has("appearance") || has("wallpaper")) {
        return "Open Themes to switch Ocean, Violet and Mint. You can also choose a theme from the paired JohnnyOS browser control center.";
    }
    if (has("sleep") || has("eco") || has("bright") || has("performance") ||
        has("speed") || has("smooth") || has("save power")) {
        return "For best battery life choose Eco: 30% brightness, dim after 10 seconds. Balanced uses 58% and Bright uses 85%. Less screen redraw makes navigation smoother.";
    }
    if (has("phone") || has("browser") || has("remote") || has("pair")) {
        return "Open Phone Remote, join the same local Wi-Fi, and enter the shown 8-character code in your browser. The Control Center lets you navigate and change settings.";
    }
    if (has("calculator") || has("math")) {
        return "Calculator has a four-column number pad. Tap to choose a key and hold Key 1 to press. It supports plus, minus, multiply, divide and decimals.";
    }
    if (has("2048") || has("memory duel") || has("puzzle")) {
        return "The Games menu now includes 2048 and Memory Duel. In 2048, tap to choose a direction and hold Key 1 to slide; Memory Duel tests your ability to repeat a colored pattern.";
    }
    if (has("portrait") || has("landscape") || has("rotate") || has("orientation")) {
        return "In JohnnyOS Settings, choose Orientation to switch between portrait and landscape home screens. All native and Bruce apps use landscape to preserve screen layout.";
    }
    if (has("mute") || has("volume") || has("sound") || has("speaker") || has("audio")) {
        return johnnySoundEnabled() ?
            "Johnny AI voice playback is ON. Open Settings or the Johnny AI Audio item to mute it. Audio defaults to OFF on a fresh setup." :
            "Johnny AI voice playback is OFF. Open Settings or the Johnny AI Audio item to enable it. Audio defaults to OFF.";
    }
    if (has("favorite") || has("reorder") || has("dock") || has("home layout")) {
        return "Open Home Layout to choose your favorite app and move icons earlier or later. Your pinned favorite is shown in the launcher dock and is placed first for quick two-button access.";
    }
    if (has("performance") || has("ram") || has("memory")) {
        return "Open Performance for free heap, PSRAM, battery and Wi-Fi status. The screen updates once per second while you view it.";
    }
    if (has("voice") || has("talk") || has("mic") || has("speech")) {
        return "Push-to-talk records audio on the device. Offline speech transcription and spoken AI replies are not available. Use the built-in topics or browser text box.";
    }
    if (has("who are you") || has("your name") || has("hello") || has("hi ")) {
        return "I'm JohnnyOS Offline Assistant! I answer built-in device questions without internet. I'm not a full conversational AI model.";
    }
    if (has("time") || has("clock") || has("date")) {
        return "Open the Clock app for device time. I do not make up the current time when the device clock has not been set.";
    }
    return "I can't answer that offline yet. Try asking about battery, Wi-Fi, Bluetooth, buttons, games, files, themes, or power. This assistant uses built-in knowledge, not a language model.";
}

void johnnyOfflineChat() {
    static const char *topics[] = {
        "What can you do?", "Battery status?", "How do buttons work?",
        "Wi-Fi status?", "How do I save power?", "What games are installed?",
        "How do I use files?", "How do I use Phone Remote?",
        "How does Air Mouse work?", "Can you hear my voice?"
    };
    constexpr int count = sizeof(topics) / sizeof(topics[0]);
    int index = 0;
    bool showAnswer = false;
    int answerPage = 0;
    bool redraw = true;
    check(NextPress); check(PrevPress); check(SelPress); check(EscPress);
    while (true) {
        johnnyPumpRemoteButtons();
        if (check(EscPress)) break;
        if (check(NextPress)) {
            if (showAnswer) answerPage++;
            else index = (index + 1) % count;
            redraw = true;
        }
        if (check(PrevPress)) {
            if (showAnswer) answerPage = max(0, answerPage - 1);
            else index = (index + count - 1) % count;
            redraw = true;
        }
        if (check(SelPress)) { showAnswer = !showAnswer; answerPage = 0; redraw = true; }
        if (redraw) {
            redraw = false;
            johnnyDrawWallpaper();
            const uint16_t white = tft.color565(246, 250, 255);
            const uint16_t soft = tft.color565(180, 201, 240);
            tft.setTextSize(1);
            tft.setTextColor(white);
            tft.drawString("Offline Assistant", 10, 7, 2);
            tft.setTextColor(tft.color565(114, 233, 194));
            tft.drawRightString("ON DEVICE", tftWidth - 9, 11, 1);
            tft.fillRoundRect(9, 29, tftWidth - 18, 86, 10, tft.color565(25, 37, 70));
            tft.setTextColor(soft);
            tft.drawString(String("TOPIC ") + String(index + 1) + "/" + String(count), 17, 36, 1);
            tft.setTextColor(white);
            tft.drawString(String(topics[index]).substring(0, 32), 17, 49, 1);
            if (!showAnswer) {
                tft.setTextColor(soft);
                tft.drawString("Hold Key 1 for answer", 17, 70, 1);
                tft.drawString("Tap keys to browse topics", 17, 86, 1);
            } else {
                String answer = johnnyOfflineReply(String(topics[index]));
                const int pages = max(1, ((int)answer.length() + 119) / 120);
                if (answerPage >= pages) answerPage = pages - 1;
                tft.setTextColor(soft);
                // Five compact lines per page to avoid truncating long replies.
                String segment = answer.substring(answerPage * 120,
                                                 min((answerPage + 1) * 120, (int)answer.length()));
                int start = 0, y = 65;
                while (start < (int)segment.length() && y <= 105) {
                    int stop = min(start + 34, (int)segment.length());
                    if (stop < (int)segment.length()) {
                        int space = segment.lastIndexOf(' ', stop);
                        if (space > start + 8) stop = space;
                    }
                    tft.drawString(segment.substring(start, stop), 17, y, 1);
                    start = stop;
                    while (start < (int)segment.length() && segment[start] == ' ') start++;
                    y += 10;
                }
            }
            tft.fillRoundRect(7, 120, tftWidth - 14, 13, 5, tft.color565(17, 28, 57));
            tft.setTextColor(soft);
            tft.drawCentreString(showAnswer
                ? "Tap: scroll  Hold 1: topics  Hold 2: back"
                : "Tap: topic  Hold 1: answer  Hold 2: back",
                                 tftWidth / 2, 123, 1);
        }
        delay(25);
    }
    check(NextPress); check(PrevPress); check(SelPress); check(EscPress);
}
#endif
