#include "buttons.h"

#include "core/display.h"
#include "core/powerSave.h"
#include "globals.h"
#if defined(JOHNNYOS_STICKS3)
#include "core/johnnyos/JohnnyApps.h"
#endif

#if defined(BUTTONS_IDF_COMPONENT) && !defined(JOHNNYOS_STICKS3)
#include <Button.h>

static Button *hal2Btn1 = nullptr;
static Button *hal2Btn2 = nullptr;
static volatile bool hal2PendingNext = false;
static volatile bool hal2PendingSel = false;
static volatile bool hal2PendingPrev = false;
static volatile bool hal2PendingEsc = false;

static void hal2OnBtn1Click(void *, void *) {
#if defined(JOHNNYOS_STICKS3)
    if (johnnySwapNavigation) hal2PendingPrev = true;
    else hal2PendingNext = true;
#else
    hal2PendingNext = true;
#endif
}
static void hal2OnBtn1Long(void *, void *) { hal2PendingSel = true; }
static void hal2OnBtn1Double(void *, void *) { hal2PendingSel = true; }
static void hal2OnBtn2Click(void *, void *) {
#if defined(JOHNNYOS_STICKS3)
    if (johnnySwapNavigation) hal2PendingNext = true;
    else hal2PendingPrev = true;
#else
    hal2PendingPrev = true;
#endif
}
static void hal2OnBtn2Long(void *, void *) { hal2PendingEsc = true; }
static void hal2OnBtn2Double(void *, void *) { hal2PendingEsc = true; }
#endif

#if defined(JOHNNYOS_STICKS3)
// Direct, debounce-filtered GPIO input for fast repeated tapping.
//
// The IDF Button component delays single-click delivery while it checks for
// a double-click. That makes repeated fast app navigation feel unresponsive.
// We intentionally reserve hold for Select / Back; two quick taps are two
// navigation steps, never an accidental double-click activation.
namespace {
struct RapidKey {
    int8_t pin = -1;
    bool raw = false;
    bool down = false;
    bool holdSent = false;
    bool waking = false;
    uint32_t edgeAt = 0;
    uint32_t downAt = 0;
};
RapidKey rapidKeys[2];
uint16_t rapidHoldMs = 420;
constexpr uint8_t RAPID_MAX_QUEUE = 24;
constexpr uint32_t RAPID_DEBOUNCE_MS = 14;
// Keep fast taps in their actual order. A FIFO also prevents a hold Select
// overtaking earlier movement events when the user taps very quickly.
uint8_t rapidEvents[RAPID_MAX_QUEUE] = {};
uint8_t rapidHead = 0, rapidTail = 0, rapidCount = 0;
enum RapidAction : uint8_t { RAPID_NEXT = 1, RAPID_PREV, RAPID_SELECT, RAPID_BACK };
void rapidPush(uint8_t action) {
    if (rapidCount == RAPID_MAX_QUEUE) return; // bounded memory, no allocations
    rapidEvents[rapidTail] = action;
    rapidTail = (rapidTail + 1) % RAPID_MAX_QUEUE;
    rapidCount++;
}
void rapidScan(RapidKey &key, int index, uint32_t now) {
    if (key.pin < 0) return;
    const bool raw = digitalRead(key.pin) == LOW;
    if (raw != key.raw) {
        key.raw = raw;
        key.edgeAt = now;
    }
    if (raw != key.down && (uint32_t)(now - key.edgeAt) >= RAPID_DEBOUNCE_MS) {
        key.down = raw;
        if (raw) {
            key.downAt = now;
            key.holdSent = false;
            // The first press simply wakes a sleeping LCD, so it cannot open
            // an unseen app or act as a hold while the display was asleep.
            key.waking = wakeUpScreen();
            AnyKeyPress = true;
        } else if (!key.holdSent && !key.waking) {
            const bool reversed = johnnySwapNavigation;
            if ((index == 0) != reversed) rapidPush(RAPID_NEXT);
            else rapidPush(RAPID_PREV);
        }
    }
    if (key.down && !key.holdSent && !key.waking &&
        (uint32_t)(now - key.downAt) >= rapidHoldMs) {
        key.holdSent = true;
        if (index == 0) rapidPush(RAPID_SELECT);
        else rapidPush(RAPID_BACK);
        AnyKeyPress = true;
    }
}
void rapidDeliver() {
    if (!rapidCount || NextPress || PrevPress || SelPress || EscPress) return;
    const uint8_t action = rapidEvents[rapidHead];
    rapidHead = (rapidHead + 1) % RAPID_MAX_QUEUE;
    rapidCount--;
    if (action == RAPID_NEXT) NextPress = true;
    else if (action == RAPID_PREV) PrevPress = true;
    else if (action == RAPID_SELECT) SelPress = true;
    else if (action == RAPID_BACK) EscPress = true;
    AnyKeyPress = true;
}
} // namespace
#endif

static void configurePin(int8_t pin, bool pullup) {
    if (pin < 0) return;
    if (pullup) pinMode(pin, INPUT_PULLUP);
    else pinMode(pin, INPUT);
}

static bool pressed(int8_t pin, bool activeHigh = false) {
    if (pin < 0) return false;
    return activeHigh ? digitalRead(pin) == HIGH : digitalRead(pin) == LOW;
}

void hal_buttons_init(const DeviceButtons &cfg, uint8_t count) {
    configurePin(cfg.btn1, cfg.pullup);
    if (count >= 3) {
        configurePin(cfg.btn2, cfg.pullup);
        configurePin(cfg.btn3, cfg.pullup);
    }
    if (count >= 5) {
        configurePin(cfg.btn4, cfg.pullup);
        configurePin(cfg.btn5, cfg.pullup);
    }
    if (count >= 6) configurePin(cfg.btn6, cfg.pullup);
}

void hal_buttons_init_2(const DeviceButtons &cfg, uint16_t long_press_ms) {
#if defined(JOHNNYOS_STICKS3)
    rapidKeys[0] = RapidKey{};
    rapidKeys[1] = RapidKey{};
    rapidHead = rapidTail = rapidCount = 0;
    rapidKeys[0].pin = cfg.btn1;
    rapidKeys[1].pin = cfg.btn2;
    // 420ms to hold feels much faster than the old 600ms IDF hold.
    // Do not silently reduce holds below 350ms: rapid taps must stay safe.
    rapidHoldMs = constrain((int)long_press_ms, 350, 500);
    for (auto &key : rapidKeys) {
        if (key.pin >= 0) {
            pinMode(key.pin, INPUT_PULLUP);
            key.raw = key.down = digitalRead(key.pin) == LOW;
            key.edgeAt = key.downAt = millis();
            key.waking = key.down; // do not activate buttons held during boot
        }
    }
#elif defined(BUTTONS_IDF_COMPONENT)
    button_config_t btn1Cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = long_press_ms,
        .short_press_time = 120,
        .gpio_button_config = {.gpio_num = cfg.btn1, .active_level = 0},
    };
    button_config_t btn2Cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = long_press_ms,
        .short_press_time = 120,
        .gpio_button_config = {.gpio_num = cfg.btn2, .active_level = 0},
    };
    hal2Btn1 = new Button(btn1Cfg);
    hal2Btn1->attachSingleClickEventCb(&hal2OnBtn1Click, nullptr);
    hal2Btn1->attachDoubleClickEventCb(&hal2OnBtn1Double, nullptr);
    hal2Btn1->attachLongPressStartEventCb(&hal2OnBtn1Long, nullptr);
    hal2Btn2 = new Button(btn2Cfg);
    hal2Btn2->attachSingleClickEventCb(&hal2OnBtn2Click, nullptr);
    hal2Btn2->attachDoubleClickEventCb(&hal2OnBtn2Double, nullptr);
    hal2Btn2->attachLongPressStartEventCb(&hal2OnBtn2Long, nullptr);
#else
    (void)cfg;
    (void)long_press_ms;
#endif
}

void hal_buttons_poll_2() {
#if defined(JOHNNYOS_STICKS3)
    const uint32_t now = millis();
    rapidScan(rapidKeys[0], 0, now);
    rapidScan(rapidKeys[1], 1, now);
    rapidDeliver();
#elif defined(BUTTONS_IDF_COMPONENT)
    if (!(hal2PendingNext || hal2PendingPrev || hal2PendingSel || hal2PendingEsc)) return;
    const bool next = hal2PendingNext;
    const bool prev = hal2PendingPrev;
    const bool select = hal2PendingSel;
    const bool back = hal2PendingEsc;
    hal2PendingNext = hal2PendingPrev = hal2PendingSel = hal2PendingEsc = false;
    if (wakeUpScreen()) return;
    AnyKeyPress = true;
    NextPress = next;
    PrevPress = prev;
    SelPress = select;
    EscPress = back;
#endif
}

void hal_buttons_poll_1(const DeviceButtons &cfg) {
    static unsigned long tm = 0;
    constexpr unsigned long kInputDebounceMs = 75;
    if (millis() - tm < kInputDebounceMs && !LongPress) return;

    checkPowerSaveTime();

    static bool buttonWasDown = false;
    static unsigned long buttonDownAt = 0;
    static uint8_t drawn = 2;
    constexpr unsigned long kSelectPressMs = 550;
    constexpr unsigned long kBackPressMs = 1200;
    constexpr unsigned long kDoublePressIntervalMs = 300;

    static unsigned long lastButtonReleaseTime = 0;
    static int clickCount = 0;
    static bool pendingNextPress = false;
    static unsigned long pendingTime = 0;

    if (pendingNextPress && millis() - pendingTime > kDoublePressIntervalMs) {
        NextPress = true;
        pendingNextPress = false;
    }

    bool buttonDown = pressed(cfg.btn1, cfg.activeHigh);

    if (buttonDown && !buttonWasDown) {
        buttonWasDown = true;
        buttonDownAt = millis();
        tm = millis();
        AnyKeyPress = true;
        LongPress = false;
        if (wakeUpScreen()) return;
    }

    if (buttonDown) {
        AnyKeyPress = true;
        if (millis() - buttonDownAt >= kSelectPressMs) {
            LongPress = true;
            if (drawn > 1) {
                tft.fillRect(tftWidth - 3, 0, 3, tftHeight, TFT_GREENYELLOW);
                tft.fillRect(0, tftHeight - 3, tftWidth, 3, TFT_GREENYELLOW);
                drawn = 1;
            }
        }
        if (millis() - buttonDownAt >= kBackPressMs && drawn > 0) {
            tft.fillRect(tftWidth - 3, 0, 3, tftHeight, TFT_RED);
            tft.fillRect(0, tftHeight - 3, tftWidth, 3, TFT_RED);
            drawn = 0;
        }
        return;
    }

    if (buttonWasDown) {
        buttonWasDown = false;
        unsigned long heldMs = millis() - buttonDownAt;
        tft.fillRect(tftWidth - 3, 0, 3, tftHeight, bruceConfig.bgColor);
        tft.fillRect(0, tftHeight - 3, tftWidth, 3, bruceConfig.bgColor);
        drawn = 2;

        if (millis() - lastButtonReleaseTime > kDoublePressIntervalMs) { clickCount = 0; }

        if (heldMs >= kBackPressMs) {
            EscPress = true;
            pendingNextPress = false;
        } else if (heldMs >= kSelectPressMs) {
            SelPress = true;
            pendingNextPress = false;
        } else {
            clickCount++;
            lastButtonReleaseTime = millis();

            if (clickCount >= 2) {
                PrevPress = true;
                clickCount = 0;
                pendingNextPress = false;
                AnyKeyPress = true;
                LongPress = false;
                return;
            } else {
                pendingNextPress = true;
                pendingTime = millis();
            }
        }
        AnyKeyPress = true;
        LongPress = false;
    }
}

void hal_buttons_poll_3(const DeviceButtons &cfg) {
    static unsigned long tm = 0;
    if (millis() - tm < 200 && !LongPress) return;

    bool prev = pressed(cfg.btn1, cfg.activeHigh);
    bool next = pressed(cfg.btn2, cfg.activeHigh);
    bool sel = pressed(cfg.btn3, cfg.activeHigh);

    bool anyPressed = prev || next || sel;
    if (anyPressed) tm = millis();
    if (anyPressed && wakeUpScreen()) return;

    AnyKeyPress = anyPressed;
    EscPress = prev && next;
    if (EscPress) return;
    PrevPress = prev;
    NextPress = next;
    SelPress = sel;
}

void hal_buttons_poll_5(const DeviceButtons &cfg) {
    static unsigned long tm = 0;
    if (millis() - tm < 200 && !LongPress) return;

    bool prev = pressed(cfg.btn1, cfg.activeHigh);
    bool next = pressed(cfg.btn2, cfg.activeHigh);
    bool up = pressed(cfg.btn3, cfg.activeHigh);
    bool down = pressed(cfg.btn4, cfg.activeHigh);
    bool sel = pressed(cfg.btn5, cfg.activeHigh);

    if (!(prev || next || up || down || sel)) return;
    tm = millis();
    if (!wakeUpScreen()) AnyKeyPress = true;
    else return;

    if (prev && next) {
        EscPress = true;
        return;
    }
    if (prev) PrevPress = true;
    if (next) NextPress = true;
    if (up) {
        UpPress = true;
        PrevPagePress = true;
    }
    if (down) {
        DownPress = true;
        NextPagePress = true;
    }
    if (sel) SelPress = true;
}

void hal_buttons_poll_6(const DeviceButtons &cfg, bool esc_on_combo_too) {
    static unsigned long tm = 0;
    if (millis() - tm < 200 && !LongPress) return;

    bool prev = pressed(cfg.btn1, cfg.activeHigh);
    bool next = pressed(cfg.btn2, cfg.activeHigh);
    bool up = pressed(cfg.btn3, cfg.activeHigh);
    bool down = pressed(cfg.btn4, cfg.activeHigh);
    bool sel = pressed(cfg.btn5, cfg.activeHigh);
    bool esc = pressed(cfg.btn6, cfg.activeHigh);

    if (!(prev || next || up || down || sel || esc)) return;
    tm = millis();
    if (!wakeUpScreen()) AnyKeyPress = true;
    else return;

    if (esc || (esc_on_combo_too && prev && next)) {
        EscPress = true;
        return;
    }
    if (prev) PrevPress = true;
    if (next) NextPress = true;
    if (up) {
        UpPress = true;
        PrevPagePress = true;
    }
    if (down) {
        DownPress = true;
        NextPagePress = true;
    }
    if (sel) SelPress = true;
}
