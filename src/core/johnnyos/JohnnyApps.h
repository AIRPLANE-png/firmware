#pragma once

#if defined(JOHNNYOS_STICKS3)
#include <MenuItemInterface.h>

// JohnnyOS native apps run only on StickS3, preserving Bruce on other boards.
// Existing Bruce Wi-Fi, Bluetooth, IR, File and Tools menus remain available.
class JohnnyMenuItem final : public MenuItemInterface {
public:
    enum Kind : uint8_t { AI, GAMES, WIFI, SETTINGS, SYSTEM, PHONE, MOUSE, MEDIA, HELP, CALCULATOR, FILES_PLUS, HOME_LAYOUT, PERFORMANCE };
    JohnnyMenuItem(const char *name, Kind kind) : MenuItemInterface(name), _kind(kind) {}
    void optionsMenu(void) override;
    void drawIcon(float scale) override;
    bool hasTheme() override { return false; }
    const String &themePath() override { static const String empty; return empty; }
private:
    Kind _kind;
};

// Navigation mapping: only the two short presses may be swapped.
// Long/double KEY1 remains Select, long/double KEY2 remains Back so
// the device always has a predictable way to exit menus.
extern volatile bool johnnySwapNavigation;
void johnnyLoadControls();
bool johnnyNavigationSwap();
void johnnySetNavigationSwap(bool reversed);
uint8_t johnnyTheme();
void johnnySetTheme(uint8_t theme);
void johnnyCycleTheme();
bool johnnySoundEnabled();
void johnnySetSoundEnabled(bool enabled); // false unless explicitly enabled
bool johnnyPortrait();
void johnnySetPortrait(bool portrait);
uint8_t johnnyWallpaperStyle();
void johnnySetWallpaperStyle(uint8_t style);
void johnnyPrepareAppScreen(); // native/legacy apps retain 240x135 landscape
void johnnyCalculator();
void johnnyFilesPlus();
void johnnyHomeLayout();
void johnnyPerformance();
void johnnyPuzzle2048();
void johnnyQuickGame();
String johnnyFavorite();
void johnnySetFavorite(const String &label);
String johnnyHomeOrder();
void johnnySetHomeOrder(const String &order);

// Stored power profiles: Eco, Balanced (default), Bright.
// Applies existing Bruce brightness/dimmer mechanisms without altering radios.
uint8_t johnnyPowerMode();
void johnnySetPowerMode(uint8_t mode);
void johnnyApplyPowerMode();
void johnnyCyclePowerMode();
uint16_t johnnyRGB(uint8_t r, uint8_t g, uint8_t b);
void johnnyDrawWallpaper();
void johnnyDrawBoot();
#endif
