#include "core/johnnyos/JohnnyExtras.h"
#if defined(JOHNNYOS_STICKS3)
#include "core/display.h"
#include "core/johnnyos/JohnnyApps.h"
#include "modules/others/mic.h"
#include "modules/others/audio.h"
#include <LittleFS.h>
#include <globals.h>

namespace {
const char *const voicePath = "/johnnyos/last-voice.wav";
void voiceCard(const String &status, const String &description) {
    johnnyDrawWallpaper();
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(248,250,255));
    tft.drawString("Johnny AI", 12, 7, 2);
    tft.setTextColor(tft.color565(165,187,239));
    tft.drawRightString("VOICE", 228, 10, 1);
    tft.fillRoundRect(12, 31, 216, 82, 14, tft.color565(28,38,76));
    tft.fillCircle(55, 71, 27, tft.color565(54,83,192));
    tft.fillCircle(48, 63, 17, tft.color565(128,116,243));
    tft.fillCircle(44, 57, 9, tft.color565(207,180,255));
    tft.setTextColor(TFT_WHITE);
    tft.drawString(status.substring(0, 21), 96, 55, 1);
    tft.setTextColor(tft.color565(176,194,230));
    tft.drawString(description.substring(0, 23), 96, 75, 1);
    tft.drawCentreString("Hold PREV to exit", 120, 121, 1);
}
} // namespace

void johnnyPushToTalk() {
#if defined(HAS_MICROPHONE)
    voiceCard("Ready to listen", "Hold NEXT button to speak");
    check(EscPress); check(SelPress);
    uint32_t startWait = millis();
    while (digitalRead(11) != LOW) {
        if (check(EscPress) || millis() - startWait > 20000) return;
        delay(30);
    }
    voiceCard("Recording...", "Release NEXT to finish");
    uint32_t bytes = 0;
    bool recorded = mic_record_wav_to_path(
        &LittleFS, voicePath, 10000, &bytes, 1.5f,
        []() -> bool { return digitalRead(11) == LOW && !check(EscPress); }
    );
    if (recorded && bytes > 44) {
        voiceCard("Voice saved", String(bytes / 1024) + " KB in LittleFS");
    } else {
        voiceCard("Record failed", "Check microphone");
    }
    delay(1200);
    check(EscPress);
#else
    voiceCard("Mic unavailable", "Not compiled for this board");
    delay(1300);
#endif
}

void johnnyPlayLastVoice() {
#if defined(HAS_SPEAKER)
    if (!LittleFS.exists(voicePath)) {
        voiceCard("Nothing recorded", "Record first");
        delay(1100);
        return;
    }
    voiceCard("Playing recording", "Through StickS3 speaker");
    playAudioFile(&LittleFS, voicePath);
#else
    voiceCard("No speaker", "Playback not available");
    delay(1100);
#endif
}
#endif
