#pragma once

#if defined(JOHNNYOS_STICKS3)
#include <Arduino.h>

// These apps are native StickS3 features, not web mockups.
void johnnyPhoneRemote();
void johnnyMouse();
void johnnyMediaRemote();
void johnnyPushToTalk();
void johnnyPlayLastVoice();

// Pump a single authorized phone command into Bruce's existing navigation
// flow, from the loop task rather than from an AsyncWebServer worker task.
bool johnnyRemotePop(uint8_t &key);
void johnnyPumpRemoteButtons(); // for standalone apps outside Bruce menu loop
// Preferences are queued by the HTTP worker, then applied on the UI task.
bool johnnyRemotePopSettings(uint8_t &nav, uint8_t &theme, uint8_t &power,
                             uint8_t &sound, uint8_t &portrait, uint8_t &wallpaper);
void johnnyRemoteUpdateBattery(int battery);

// Compact on-device assistant: curated intent matching, no cloud or LLM.
String johnnyOfflineReply(const String &question);
void johnnyOfflineChat();
#endif
